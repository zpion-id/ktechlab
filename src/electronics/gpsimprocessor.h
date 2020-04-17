/***************************************************************************
 *   Copyright (C) 2005 by David Saxton                                    *
 *   david@bluehaze.org                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "config.h"
#ifndef NO_GPSIM

#ifndef GPSIMPROCESSOR_H
#define GPSIMPROCESSOR_H

#include "sourceline.h"

#include <qmap.h>
// #include <q3valuevector.h>
#include <qobject.h>
#include <qlist.h>
#include <QVector>

class DebugLine;
class GpsimProcessor;
class MicroInfo;
class pic_processor; // from gpsim
class Register;
class RegisterMemoryAccess;

typedef QMap<SourceLine, SourceLine> SourceLineMap;
typedef QList<int> IntList;


class DebugLine : public SourceLine
{
	public:
		DebugLine();
		/// @param fileName a path to a file in the local filesystem
		DebugLine( const QString & fileName, int line );
		/**
		 * Whether or not to break when we reach this line.
		 */
		bool isBreakpoint() const { return m_bIsBreakpoint; }
		/**
		 * Set whether or not to break when we reach this line.
		 */
		void setBreakpoint( bool breakpoint ) { m_bIsBreakpoint = breakpoint; }
		/**
		 * Used for efficiency purposes by GpsimProcessor. Sets a flag.
		 */
		void markAsDeleted() { m_bMarkedAsDeleted = true; }
		/**
		 * Used for efficiency purposes by GpsimProcessor.
		 */
		bool markedAsDeleted() const { return m_bMarkedAsDeleted; }

	protected:
		bool m_bIsBreakpoint;
		bool m_bMarkedAsDeleted;

	private:
		DebugLine( const DebugLine & dl );
		DebugLine & operator = ( const DebugLine & dl );
};


/**
@short Stores info from gpsim register, used to hide gpsim interface
@author David Saxton
*/
class RegisterInfo : public QObject
{
	Q_OBJECT
	public:
		RegisterInfo( Register * reg );

		enum RegisterType
		{
			Invalid,
			Generic,
			File,
			SFR,
			Breakpoint
		};

		RegisterType type() const { return m_type; }
		QString name() const { return m_name; }
		unsigned value() const;
		static QString toString( RegisterType type );

		/**
		 * Checks to see if the value has changed; if so, emit new value.
		 */
		void update();

	signals:
		void valueChanged( unsigned newValue );

	protected:
		QString m_name;
		RegisterType m_type;
		Register * m_pRegister;
		unsigned m_prevEmitValue;
};


/**
@short Stores information about a set of registers, used to hide gpsim interface.
@author David Saxton
*/
class RegisterSet
{
	public:
		RegisterSet( pic_processor * picProcessor );
		~RegisterSet();

		/**
		 * Calls update for each RegisterInfo in this set.
		 */
		void update();
		/**
		 * Returns the number of registers.
		 */
		unsigned size() const { return m_registers.size(); }

		RegisterInfo * fromAddress( unsigned address );
		RegisterInfo * fromName( const QString & name );

	protected:
		typedef QMap< QString, RegisterInfo * > RegisterInfoMap;
		RegisterInfoMap m_nameToRegisterMap;
		QVector< RegisterInfo * > m_registers;
};


/**
@author David Saxton
*/
class GpsimDebugger : public QObject
{
	friend class GpsimProcessor;
	Q_OBJECT

	public:
		enum Type
		{
			AsmDebugger = 0,
			HLLDebugger = 1
		};

		GpsimDebugger( Type type, GpsimProcessor * gpsim );
		~GpsimDebugger() override;

		GpsimProcessor * gpsim() const { return m_pGpsim; }

		/**
		 * When an assembly file was generated by a high level language compiler
		 * like SDCC, it will insert markers like ";#CSRC" that show which line
		 * of source-code generated the given set of assembly instructions. This
		 * matches up the assembly file lines with the associated source file
		 * lines.
		 * @param sourceFile the path to an assembly file in the local filesystem
		 * @param assemblyFile the path to a source file in the local filesystem
		 */
		void associateLine( const QString & sourceFile, int sourceLine, const QString & assemblyFile, int assemblyLine );
		/**
		 * Check to see if we've hit a breakpoint or similar; if so, this
		 * function will stop the execution of the PIC program.
		 */
		void checkForBreak();
		/**
		 * Sets the breakpoints used for the given file to exactly those that
		 * are contained in this list. Breakpoints for other files are not
		 * affected.
		 * @param path the location of the file (which gpsim must recognise).
		 */
		void setBreakpoints( const QString & path, const IntList & lines );
		/**
		 * Sets / removes the breakpoint at the given line
		 */
		void setBreakpoint( const QString & path, int line, bool isBreakpoint );
		/**
		 * Returns the current source line that gpsim is at. By default, this
		 * will be the corresponding assembly line. That can be overwritten
		 * using mapAddressBlockToLine.
		 */
		SourceLine currentLine();
		/**
		 * Returns a pointer to the debug info for the current line.
		 */
		DebugLine * currentDebugLine();
		/**
		 * @return the program address for the given line (or -1 if no such
		 * line).
		 */
		int programAddress( const QString & path, int line );
		/**
		 * Step into the next program line.
		 */
		void stepInto();
		/**
		 * Step over the next program instruction. If we are currently running,
		 * this function will do nothing. Otherwise, it will record the current
		 * stack level, step, and if the new stack level is <= the initial level
		 * then return - otherwise, this processor will set a breakpoint for
		 * stack levels <= initial, and go to running mode.
		 */
		void stepOver();
		/**
		 * Similar to stepOver, except we break when the stack level becomes <
		 * the initial stack level (instead of <= initial).
		 */
		void stepOut();

	signals:
		/**
		 * Emitted when a line is reached. By default, this is the line of the
		 * input assembly file; however, the line associated with an address in
		 * the PIC memory can be changed with mapAddressBlockToLine.
		 */
		void lineReached( const SourceLine & sourceLine );

	protected slots:
		void gpsimRunningStatusChanged( bool isRunning );

	protected:
		void initAddressToLineMap();
		void stackStep( int dl );
		void emitLineReached();

		int m_stackLevelLowerBreak; // Set by step-over, for when the stack level decreases to the one given
		SourceLine m_previousAtLineEmit; // Used for working out whether we should emit a new line reached signal
		DebugLine ** m_addressToLineMap;
		DebugLine * m_pBreakFromOldLine;
		GpsimProcessor * m_pGpsim;
		Type m_type;
		unsigned m_addressSize;
		SourceLineMap m_sourceLineMap; // assembly <--> High level language
};


/**
@author David Saxton
*/
class GpsimProcessor : public QObject
{
	friend class GpsimDebugger;
	Q_OBJECT

	public:
		/**
		 * Create a new gpsim processor. After calling this constructor, you
		 * should always call codLoadStatus() to ensure that the cod file was
		 * loaded successfully.
		 */
		GpsimProcessor( QString symbolFile, QObject *parent = nullptr );
		~GpsimProcessor() override;

		void setDebugMode( GpsimDebugger::Type mode ) { m_debugMode = mode; }
		GpsimDebugger * currentDebugger() const { return m_pDebugger[m_debugMode]; }

		enum CodLoadStatus
		{
			CodSuccess,
			CodFileNotFound,
			CodUnrecognizedProcessor,
			CodFileNameTooLong,
			CodLstNotFound,
			CodBadFile,
			CodFileUnreadable,
			CodFailure,
			CodUnknown // Should never be this, but just in case load_symbol_file returns something funny
		};

		enum InstructionType
		{
			LiteralOp,
			BitOp,
			RegisterOp,
			UnknownOp
		};

		/**
		 * @return status of opening the COD file
		 * @see displayCodLoadStatus
		 */
		CodLoadStatus codLoadStatus() const { return m_codLoadStatus; }
		/**
		 * Popups a messagebox to the user according to the CodLoadStatus. Will
		 * only popup a messagebox if the CodLoadStatus wasn't CodSuccess.
		 */
		void displayCodLoadStatus();
		/**
		 * Returns a list of source files for the currently running program.
		 * Each entry is a path in the local filesystem.
		 */
		QStringList sourceFileList();
		/**
		 * Set whether or not to run gpsim. (i.e. whether or not the step
		 * function should do anything when called with force=false).
		 */
		void setRunning( bool run );
		/**
		 * Returns true if running (currently simulating), else gpsim is paused.
		 */
		bool isRunning() const { return m_bIsRunning; }
		/**
		 * Execute the next program instruction. If we are not in a running
		 * mode, then this function will do nothing.
		 */
		void executeNext();
		/**
		 * Reset all parts of the simulation. Gpsim will not run until
		 * setRunning(true) is called. Breakpoints are not affected.
		 */
		void reset();
		/**
		 * Returns the microinfo describing this processor.
		 */
		MicroInfo * microInfo() const;

		pic_processor * picProcessor() const { return m_pPicProcessor; }
		unsigned programMemorySize() const;
		RegisterSet * registerMemory() const { return m_pRegisterMemory; }
		/**
		 * @return the instruction type at the given address.
		 */
		InstructionType instructionType( unsigned address );
		/**
		 * @return the address of the operand's register at address if the
		 * instruction at address is a register operation, and -1 otherwise.
		 */
		int operandRegister( unsigned address );
		/**
		 * @return the literal if the instruction at address is a literal
		 * operation, and -1 otherwise.
		 */
		int operandLiteral( unsigned address );

		//BEGIN Convenience functions for PIC files
		enum ProgramFileValidity { DoesntExist, IncorrectType, Valid };
		/**
		 * @return information on the validity of the given program file (either
		 * DoesntExist, IncorrectType, or Valid).
		 * @see static QString generateSymbolFile
		 */
		static ProgramFileValidity isValidProgramFile( const QString & programFile );
		/**
		 * Converts the file at programFile to a Symbol file for emulation,
		 * and returns that symbol file's path
		 * @param fileName The full url to the file
		 * @param receiver The slot to connect the assembled signal to
		 * @see static bool isValidProgramFile( const QString &programFile )
		 */
		static QString generateSymbolFile( const QString &fileName, QObject *receiver, const char *successMember, const char * failMember = nullptr );
		/**
		 *Compile microbe to output to the given filename
		 */
		static void compileMicrobe( const QString &filename, QObject *receiver, const char * successMember, const char * failMember = nullptr );
		//END convenience functions for PIC files

	signals:
		/**
		 * Emitted when the running status of gpsim changes.
		 */
		void runningStatusChanged( bool isRunning );

	protected:
		/**
		 * Calls emitLineReached for each debugger.
		 */
		void emitLineReached();

		pic_processor * m_pPicProcessor;
		CodLoadStatus m_codLoadStatus;
		const QString m_symbolFile;
		RegisterSet * m_pRegisterMemory;
		GpsimDebugger::Type m_debugMode;
		GpsimDebugger * m_pDebugger[2]; // Asm, HLL

		/**
		 * We are called effectively for each cycle of the cycle of the
		 * processor. This value is used as some instructions (e.g. goto) take
		 * two cycles to execute, and so we must ignore one cycle to ensure
		 * realtime simulation.
		 */
		bool m_bCanExecuteNextCycle;

	private:
		bool m_bIsRunning;
};

#endif

#endif // !NO_GPSIM
