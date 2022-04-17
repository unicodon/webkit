//
// Copyright 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// SwapChainPanelNativeWindow.cpp: NativeWindow for managing ISwapChainPanel native window types.

#include "libANGLE/renderer/d3d/d3d11/winui3/SwapChainPanelNativeWindow.h"

#include <math.h>
#include <algorithm>
#include <winrt/Microsoft.UI.Dispatching.h>

using namespace ABI::Windows::Foundation;
using namespace ABI::Windows::Foundation::Collections;

namespace rx
{
SwapChainPanelNativeWindow::~SwapChainPanelNativeWindow()
{
    unregisterForSizeChangeEvents();
}

template <typename CODE>
HRESULT RunOnUIThread(CODE &&code, winrt::Microsoft::UI::Dispatching::DispatcherQueue dispatcherQueue)
{
    HRESULT result = S_OK;

    bool hasThreadAccess = dispatcherQueue.HasThreadAccess();

    if (hasThreadAccess)
    {
        return code();
    }
    else
    {
        HANDLE hEvent = CreateEventEx(nullptr, nullptr, CREATE_EVENT_MANUAL_RESET, EVENT_ALL_ACCESS);
        if (!hEvent)
        {
            return E_FAIL;
        }

        HRESULT codeResult = E_FAIL;
        if (!dispatcherQueue.TryEnqueue([&codeResult, &code, hEvent] {
            codeResult = code();
            SetEvent(hEvent);
            return S_OK;
            }))
        {
            CloseHandle(hEvent);
            return E_FAIL;
        }

        auto waitResult = WaitForSingleObjectEx(hEvent, 10 * 1000, true);
        CloseHandle(hEvent);
        if (waitResult != WAIT_OBJECT_0)
        {
            // Wait 10 seconds before giving up. At this point, the application is in an
            // unrecoverable state (probably deadlocked). We therefore terminate the application
            // entirely. This also prevents stack corruption if the async operation is eventually
            // run.
            ERR()
                << "Timeout waiting for async action on UI thread. The UI thread might be blocked.";
            std::terminate();
            return E_FAIL;
        }

        return codeResult;
    }
}

bool SwapChainPanelNativeWindow::initialize(
    EGLNativeWindowType window,
    const winrt::Windows::Foundation::Collections::IPropertySet &propertySet)
{
    winrt::com_ptr<IInspectable> win;
    win.copy_from(window);
    SIZE swapChainSize         = {};
    HRESULT result             = S_OK;

    // IPropertySet is an optional parameter and can be null.
    // If one is specified, cache as an IMap and read the properties
    // used for initial host initialization.
    if (propertySet)
    {
        // The EGLRenderSurfaceSizeProperty is optional and may be missing. The IPropertySet
        // was prevalidated to contain the EGLNativeWindowType before being passed to
        // this host.
        result = GetOptionalSizePropertyValue(propertySet, EGLRenderSurfaceSizeProperty,
                                              swapChainSize, mSwapChainSizeSpecified);
        if (FAILED(result))
        {
            return false;
        }

        // The EGLRenderResolutionScaleProperty is optional and may be missing. The IPropertySet
        // was prevalidated to contain the EGLNativeWindowType before being passed to
        // this host.
        result = GetOptionalSinglePropertyValue(propertySet, EGLRenderResolutionScaleProperty,
                                                mSwapChainScale, mSwapChainScaleSpecified);
        if (FAILED(result))
        {
            return false;
        }

        if (!mSwapChainScaleSpecified)
        {
            // Default value for the scale is 1.0f
            mSwapChainScale = 1.0f;
        }

        // A EGLRenderSurfaceSizeProperty and a EGLRenderResolutionScaleProperty can't both be
        // specified
        if (mSwapChainScaleSpecified && mSwapChainSizeSpecified)
        {
            ERR() << "It is invalid to specify both an EGLRenderSurfaceSizeProperty and a "
                     "EGLRenderResolutionScaleProperty.";
            return false;
        }
    }

    if (SUCCEEDED(result))
    {
        auto swapChainPanel = win.try_as<winrt::Microsoft::UI::Xaml::Controls::SwapChainPanel>();
        if (swapChainPanel)
        {
            mSwapChainPanel = swapChainPanel;
        }
        else
        {
            result = E_NOINTERFACE;
        }
    }

    if (SUCCEEDED(result))
    {
        // If a swapchain size is specfied, then the automatic resize
        // behaviors implemented by the host should be disabled.  The swapchain
        // will be still be scaled when being rendered to fit the bounds
        // of the host.
        // Scaling of the swapchain output needs to be handled by the
        // host for swapchain panels even though the scaling mode setting
        // DXGI_SCALING_STRETCH is configured on the swapchain.
        if (mSwapChainSizeSpecified)
        {
            mClientRect = {0, 0, swapChainSize.cx, swapChainSize.cy};
        }
        else
        {
            winrt::Windows::Foundation::Size swapChainPanelSize;
            result = GetSwapChainPanelSize(mSwapChainPanel, &swapChainPanelSize);

            if (SUCCEEDED(result))
            {
                // Update the client rect to account for any swapchain scale factor
                mClientRect = clientRect(swapChainPanelSize);
            }
        }
    }

    if (SUCCEEDED(result))
    {
        mNewClientRect     = mClientRect;
        mClientRectChanged = false;
        return registerForSizeChangeEvents();
    }

    return false;
}

bool SwapChainPanelNativeWindow::registerForSizeChangeEvents()
{
    auto that = this->shared_from_this();

    HRESULT result = RunOnUIThread(
        [that] {
            that->mSizeChangedEventToken = that->mSwapChainPanel.SizeChanged(
                [that](winrt::Windows::Foundation::IInspectable const &sender,
                       winrt::Microsoft::UI::Xaml::SizeChangedEventArgs const &e) {
                    that->setNewClientSize(e.NewSize());
            });
            return S_OK;
        },
        mSwapChainPanel.DispatcherQueue());

    if (SUCCEEDED(result))
    {
        return true;
    }

    return false;
}

void SwapChainPanelNativeWindow::unregisterForSizeChangeEvents()
{
    auto that = this->shared_from_this();

    HRESULT result = RunOnUIThread(
        [that, token = mSizeChangedEventToken] {
            that->mSwapChainPanel.SizeChanged(token);
            return 0;
        },
        mSwapChainPanel.DispatcherQueue());
    mSizeChangedEventToken = winrt::event_token();
}

HRESULT SwapChainPanelNativeWindow::createSwapChain(ID3D11Device *device,
                                                    IDXGIFactory2 *factory,
                                                    DXGI_FORMAT format,
                                                    unsigned int width,
                                                    unsigned int height,
                                                    bool containsAlpha,
                                                    IDXGISwapChain1 **swapChain)
{
    if (device == nullptr || factory == nullptr || swapChain == nullptr || width == 0 ||
        height == 0)
    {
        return E_INVALIDARG;
    }

    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {0};
    swapChainDesc.Width                 = width;
    swapChainDesc.Height                = height;
    swapChainDesc.Format                = format;
    swapChainDesc.Stereo                = FALSE;
    swapChainDesc.SampleDesc.Count      = 1;
    swapChainDesc.SampleDesc.Quality    = 0;
    swapChainDesc.BufferUsage =
        DXGI_USAGE_SHADER_INPUT | DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_BACK_BUFFER;
    swapChainDesc.BufferCount = 2;
    swapChainDesc.SwapEffect  = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    swapChainDesc.Scaling     = DXGI_SCALING_STRETCH;
    swapChainDesc.AlphaMode =
        containsAlpha ? DXGI_ALPHA_MODE_PREMULTIPLIED : DXGI_ALPHA_MODE_IGNORE;

    *swapChain = nullptr;

    winrt::com_ptr<IDXGISwapChain1> newSwapChain;
    winrt::com_ptr<ISwapChainPanelNative> swapChainPanelNative;
    winrt::Windows::Foundation::Size currentPanelSize = {};

    HRESULT result = factory->CreateSwapChainForComposition(device, &swapChainDesc, nullptr,
                                                            newSwapChain.put());

    if (SUCCEEDED(result))
    {
        result = winrt::get_unknown(mSwapChainPanel)->QueryInterface(__uuidof(ISwapChainPanelNative), swapChainPanelNative.put_void());
    }

    if (SUCCEEDED(result))
    {
        result = RunOnUIThread(
            [swapChainPanelNative, newSwapChain] {
                return swapChainPanelNative->SetSwapChain(newSwapChain.get());
            },
            mSwapChainPanel.DispatcherQueue());
    }

    if (SUCCEEDED(result))
    {
        // The swapchain panel host requires an instance of the swapchain set on the SwapChainPanel
        // to perform the runtime-scale behavior.  This swapchain is cached here because there are
        // no methods for retreiving the currently configured on from ISwapChainPanelNative.
        mSwapChain = newSwapChain;
        newSwapChain.copy_to(swapChain);
    }

    // If the host is responsible for scaling the output of the swapchain, then
    // scale it now before returning an instance to the caller.  This is done by
    // first reading the current size of the swapchain panel, then scaling
    if (SUCCEEDED(result))
    {
        if (mSwapChainSizeSpecified || mSwapChainScaleSpecified)
        {
            result = GetSwapChainPanelSize(mSwapChainPanel, &currentPanelSize);

            // Scale the swapchain to fit inside the contents of the panel.
            if (SUCCEEDED(result))
            {
                result = scaleSwapChain(currentPanelSize, mClientRect);
            }
        }
    }

    return result;
}

HRESULT SwapChainPanelNativeWindow::scaleSwapChain(
    const winrt::Windows::Foundation::Size &windowSize,
    const RECT &clientRect)
{
    Size renderScale = {windowSize.Width / (float)clientRect.right,
                        windowSize.Height / (float)clientRect.bottom};
    // Setup a scale matrix for the swap chain
    DXGI_MATRIX_3X2_F scaleMatrix = {};
    scaleMatrix._11               = renderScale.Width;
    scaleMatrix._22               = renderScale.Height;

    winrt::com_ptr<IDXGISwapChain2> swapChain2;
    HRESULT result = mSwapChain.as(__uuidof(IDXGISwapChain2), swapChain2.put_void());
    if (SUCCEEDED(result))
    {
        result = swapChain2->SetMatrixTransform(&scaleMatrix);
    }

    return result;
}

HRESULT GetSwapChainPanelSize(
    winrt::Microsoft::UI::Xaml::Controls::SwapChainPanel& swapChainPanel,
                              winrt::Windows::Foundation::Size *windowSize)
{
    HRESULT result = RunOnUIThread([swapChainPanel, windowSize] { 
        *windowSize = swapChainPanel.RenderSize();
        return 0;
        },
        swapChainPanel.DispatcherQueue());
    return result;
}

}  // namespace rx
