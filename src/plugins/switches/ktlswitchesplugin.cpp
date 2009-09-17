/***************************************************************************
 *   Copyright (C) 2008 Julian Bäume <julian@svg4all.de>                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "ktlswitchesplugin.h"

#include "rotaryswitch.h"
#include "interfaces/component/icomponent.h"
#include "interfaces/component/icomponentplugin.h"
#include "interfaces/idocumentplugin.h"

#include <shell/core.h>
#include <interfaces/iplugincontroller.h>
#include <KGenericFactory>
#include <KAboutData>

K_PLUGIN_FACTORY(KTLSwitchesPluginFactory, registerPlugin<KTLSwitchesPlugin>(); )
K_EXPORT_PLUGIN(KTLSwitchesPluginFactory(KAboutData("ktlswitches","ktlswitches", ki18n("KTechLab Switch Components"), "0.1", ki18n("Provide a set of standard switch components"), KAboutData::License_LGPL)))


class KTLSwitchesFactory: public KTechLab::IComponentFactory
{
public:
    KTLSwitchesFactory()
    {
        addSupportedComponent( RotarySwitch::metaData() );
    }

    virtual KTechLab::IComponent * create( const QString &name )
    {
        return 0;
    }

};

KTLSwitchesPlugin::KTLSwitchesPlugin( QObject *parent, const QVariantList& args )
    :   KTechLab::IComponentPlugin( KTLSwitchesPluginFactory::componentData(), parent ),
        m_componentFactory( new KTLSwitchesFactory() )
{

    init();
}

void KTLSwitchesPlugin::init()
{
    QStringList constraints;
    constraints << QString("'%1' in [X-KDevelop-SupportedMimeTypes]").arg("application/x-circuit");
    QList<KDevelop::IPlugin*> plugins = KDevelop::Core::self()->pluginController()->allPluginsForExtension( "KTLDocument", constraints );
    if (plugins.isEmpty()) {
        return;
    }
    KTechLab::IDocumentPlugin *plugin = qobject_cast<KTechLab::IDocumentPlugin*>( plugins.first() );

    plugin->registerComponentFactory( m_componentFactory );
}

KTLSwitchesPlugin::~KTLSwitchesPlugin()
{
}

void KTLSwitchesPlugin::unload()
{
    //me be, we should unregister our components at the circuit-plugin
}

#include "ktlswitchesplugin.moc"

// vim: sw=4 sts=4 et tw=100
