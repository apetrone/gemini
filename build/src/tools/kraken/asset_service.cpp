// -------------------------------------------------------------
// Copyright (C) 2014- Adam Petrone
// All rights reserved.

// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//      * Redistributions of source code must retain the above copyright notice,
//      this list of conditions and the following disclaimer.

//      * Redistributions in binary form must reproduce the above copyright notice,
//      this list of conditions and the following disclaimer in the documentation
//      and/or other materials provided with the distribution.

// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
//       SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// -------------------------------------------------------------

#include "asset_service.h"

#include <QDir>

void AssetService::start_watching_assets(const QString& project_path)
{
    QString asset_path = project_path;
    asset_path.append(QDir::separator());
    asset_path.append("assets");

    // startup blacksmith
    QString old_path = QDir::currentPath();

    // TODO: if in development
    QDir dir = QDir(old_path);
    dir.cd("../../../");

    QString engine_root = dir.path() + QDir::separator();
    QDir::setCurrent(engine_root);

    QString script_path = engine_root + "tools/blacksmith/blacksmith.py";
    QString config_path = engine_root + "tools/conf/blacksmith/desktop_monitor.conf";
    QString command = engine_root + "tools/env/bin/python " + script_path + " -c " + config_path + " -s " + asset_path;

    log->log(command);

    // close the process if it's already open...
    stop_watching_assets();

    connect(&blacksmith, SIGNAL(readyReadStandardOutput()), this, SLOT(on_standard_output()));
    connect(&blacksmith, SIGNAL(readyReadStandardError()), this, SLOT(on_standard_error()));

    blacksmith.start(command);

    // restore the old path
    QDir::setCurrent(old_path);
}

void AssetService::stop_watching_assets()
{
    if (blacksmith.isOpen())
    {
        disconnect(&blacksmith, SIGNAL(readyReadStandardOutput()), this, SLOT(on_standard_output()));
        disconnect(&blacksmith, SIGNAL(readyReadStandardError()), this, SLOT(on_standard_error()));
        blacksmith.close();
    }
}

void AssetService::on_standard_output()
{
    log->log(blacksmith.readAllStandardOutput());
}

void AssetService::on_standard_error()
{
    log->log(blacksmith.readAllStandardError());
}
