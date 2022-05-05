//
// Copyright 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// InspectableNativeWindow.h: Host specific implementation interface for
// managing IInspectable native window types.

#ifndef LIBANGLE_RENDERER_D3D_D3D11_WINUI3_INSPECTABLENATIVEWINDOW_H_
#define LIBANGLE_RENDERER_D3D_D3D11_WINUI3_INSPECTABLENATIVEWINDOW_H_

#include "common/debug.h"
#include "common/platform.h"

#include "angle_windowsstore.h"

#include <EGL/eglplatform.h>

#include <windows.applicationmodel.core.h>

#undef GetCurrentTime
#include <microsoft.ui.xaml.media.dxinterop.h>

#include <winrt/base.h>
#include <winrt/Microsoft.UI.Xaml.h>
#include <winrt/Microsoft.UI.Xaml.Controls.h>
#include <winrt/Microsoft.UI.Xaml.Controls.Primitives.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.UI.Xaml.Interop.h>

namespace rx
{
class InspectableNativeWindow
{
  public:
    InspectableNativeWindow()
        : mSupportsSwapChainResize(true),
          mSwapChainSizeSpecified(false),
          mSwapChainScaleSpecified(false),
          mSwapChainScale(1.0f),
          mClientRectChanged(false),
          mClientRect({0, 0, 0, 0}),
          mNewClientRect({0, 0, 0, 0})
    {
        mSizeChangedEventToken.value = 0;
    }
    virtual ~InspectableNativeWindow() {}

    virtual bool initialize(EGLNativeWindowType window,
                            const winrt::Windows::Foundation::Collections::IPropertySet& propertySet) = 0;
    virtual HRESULT createSwapChain(ID3D11Device *device,
                                    IDXGIFactory2 *factory,
                                    DXGI_FORMAT format,
                                    unsigned int width,
                                    unsigned int height,
                                    bool containsAlpha,
                                    IDXGISwapChain1 **swapChain)                   = 0;

    bool getClientRect(RECT *rect)
    {
        if (mClientRectChanged)
        {
            mClientRect = mNewClientRect;
        }

        *rect = mClientRect;

        return true;
    }

    // setNewClientSize is used by the WinRT size change handler. It isn't used by the rest of
    // ANGLE.
    void setNewClientSize(const winrt::Windows::Foundation::Size &newWindowSize)
    {
        // If the client doesn't support swapchain resizing then we should have already unregistered
        // from size change handler
        ASSERT(mSupportsSwapChainResize);

        if (mSupportsSwapChainResize)
        {
            // If the swapchain size was specified then we should ignore this call too
            if (!mSwapChainSizeSpecified)
            {
                mNewClientRect     = clientRect(newWindowSize);
                mClientRectChanged = true;

                // If a scale was specified, then now is the time to apply the scale matrix for the
                // new swapchain size and window size
                if (mSwapChainScaleSpecified)
                {
                    scaleSwapChain(newWindowSize, mNewClientRect);
                }
            }

            // Even if the swapchain size was fixed, the window might have changed size.
            // In this case, we should recalculate the scale matrix to account for the new window
            // size
            if (mSwapChainSizeSpecified)
            {
                scaleSwapChain(newWindowSize, mClientRect);
            }
        }
    }

  protected:
    virtual HRESULT scaleSwapChain(const winrt::Windows::Foundation::Size &windowSize,
                                   const RECT &clientRect) = 0;
    RECT clientRect(const winrt::Windows::Foundation::Size &size);

    bool mSupportsSwapChainResize;  // Support for IDXGISwapChain::ResizeBuffers method
    bool mSwapChainSizeSpecified;   // If an EGLRenderSurfaceSizeProperty was specified
    bool mSwapChainScaleSpecified;  // If an EGLRenderResolutionScaleProperty was specified
    float mSwapChainScale;  // The scale value specified by the EGLRenderResolutionScaleProperty
                            // property
    RECT mClientRect;
    RECT mNewClientRect;
    bool mClientRectChanged;

    winrt::event_token mSizeChangedEventToken;
};

bool IsSwapChainPanel(EGLNativeWindowType window,
                      winrt::Microsoft::UI::Xaml::Controls::SwapChainPanel *swapChainPanel = nullptr);

bool IsEGLConfiguredPropertySet(
    EGLNativeWindowType window,
    winrt::Windows::Foundation::Collections::IPropertySet *propertySet = nullptr,
    EGLNativeWindowType *eglNativeWindow                               = nullptr);

bool GetOptionalPropertyValue(
    const winrt::Windows::Foundation::Collections::IPropertySet &propertySet,
    const wchar_t *propertyName,
    bool &hasKey,
    winrt::Windows::Foundation::IInspectable &propertyValue);

bool GetOptionalSizePropertyValue(
    const winrt::Windows::Foundation::Collections::IPropertySet &propertySet,
    const wchar_t *propertyName,
    SIZE &value,
    bool &valueExists);

bool GetOptionalSinglePropertyValue(
    const winrt::Windows::Foundation::Collections::IPropertySet &propertySet,
    const wchar_t *propertyName,
    float &value,
    bool &valueExists);


}  // namespace rx

#endif  // LIBANGLE_RENDERER_D3D_D3D11_WINUI3_INSPECTABLENATIVEWINDOW_H_
