//
// Copyright 2016 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// NativeWindow11WinRT.cpp: NativeWindow base class for managing IInspectable native window types.

#include "libANGLE/renderer/d3d/d3d11/winui3/NativeWindow11WinUI3.h"

#include "libANGLE/renderer/d3d/d3d11/renderer11_utils.h"
#include "libANGLE/renderer/d3d/d3d11/winui3/InspectableNativeWindow.h"
#include "libANGLE/renderer/d3d/d3d11/winui3/SwapChainPanelNativeWindow.h"

namespace rx
{
NativeWindow11WinUI3::NativeWindow11WinUI3(EGLNativeWindowType window, bool hasAlpha)
    : NativeWindow11(window), mHasAlpha(hasAlpha)
{}

bool NativeWindow11WinUI3::initialize()
{
    EGLNativeWindowType window = getNativeWindow();

    // If the native window type is a IPropertySet, extract the
    // EGLNativeWindowType (IInspectable) and initialize the
    // proper host with this IPropertySet.
    winrt::Windows::Foundation::Collections::IPropertySet propertySet;
    winrt::com_ptr<IInspectable> eglNativeWindow;
    winrt::Microsoft::UI::Xaml::Controls::SwapChainPanel swapChainPanel;
    if (IsEGLConfiguredPropertySet(window, &propertySet, eglNativeWindow.put()))
    {
        // A property set was found and the EGLNativeWindowType was
        // retrieved. The mWindow member of the host to must be updated
        // to use the EGLNativeWindowType specified in the property set.
        // mWindow is treated as a raw pointer not an AddRef'd interface, so
        // the old mWindow does not need a Release() before this assignment.
        window = eglNativeWindow.get();
    }
    
    if (IsSwapChainPanel(window, &swapChainPanel))
    {
        mImpl = std::make_shared<SwapChainPanelNativeWindow>();
        if (mImpl)
        {
            return mImpl->initialize(window, propertySet);
        }
    }
    else
    {
        ERR() << "Invalid IInspectable EGLNativeWindowType detected. Valid IInspectables include "
                 "ICoreWindow, ISwapChainPanel and IPropertySet";
    }

    return false;
}

bool NativeWindow11WinUI3::getClientRect(LPRECT rect) const
{
    if (mImpl)
    {
        return mImpl->getClientRect(rect);
    }

    return false;
}

bool NativeWindow11WinUI3::isIconic() const
{
    return false;
}

HRESULT NativeWindow11WinUI3::createSwapChain(ID3D11Device *device,
                                             IDXGIFactory *factory,
                                             DXGI_FORMAT format,
                                             UINT width,
                                             UINT height,
                                             UINT samples,
                                             IDXGISwapChain **swapChain)
{
    if (samples > 1)
    {
        // Multisample not implemented for WinRT window types
        return E_NOTIMPL;
    }

    if (mImpl)
    {
        IDXGIFactory2 *factory2     = d3d11::DynamicCastComObject<IDXGIFactory2>(factory);
        IDXGISwapChain1 *swapChain1 = nullptr;
        HRESULT result =
            mImpl->createSwapChain(device, factory2, format, width, height, mHasAlpha, &swapChain1);
        SafeRelease(factory2);
        *swapChain = static_cast<IDXGISwapChain *>(swapChain1);
        return result;
    }

    return E_UNEXPECTED;
}

void NativeWindow11WinUI3::commitChange() {}

// static
bool NativeWindow11WinUI3::IsValidNativeWindow(EGLNativeWindowType window)
{
    // A Valid EGLNativeWindowType IInspectable can only be:
    //
    // ISwapChainPanel
    // IPropertySet
    //
    // Anything else will be rejected as an invalid IInspectable.
    return IsSwapChainPanel(window) || IsEGLConfiguredPropertySet(window);
}

}  // namespace rx
