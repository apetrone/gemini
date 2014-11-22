// -------------------------------------------------------------
// Copyright (C) 2014- Adam Petrone

// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM,OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
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
