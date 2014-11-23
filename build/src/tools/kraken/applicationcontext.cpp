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
#include "applicationcontext.h"

#include <gemini/typedefs.h>

#include <renderer/renderer.h>

ApplicationContext::ApplicationContext() :
    log_instance(QSharedPointer<LogService>(new LogService())),
    asset_instance(QSharedPointer<AssetService>(new AssetService(log_instance)))
{
}

ApplicationContext::~ApplicationContext()
{
    asset_instance->stop_watching_assets();
}

void ApplicationContext::startup()
{
    renderer::RenderSettings settings;
    int render_result = renderer::startup(renderer::Default, settings);

    log()->log(QString("initialized renderer %1").arg(render_result));
}

void ApplicationContext::shutdown()
{
    renderer::shutdown();
}
