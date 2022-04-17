//
// Copyright 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// SwapChainPanelNativeWindow.h: NativeWindow for managing ISwapChainPanel native window types.

#ifndef LIBANGLE_RENDERER_D3D_D3D11_WINUI3_SWAPCHAINPANELNATIVEWINDOW_H_
#define LIBANGLE_RENDERER_D3D_D3D11_WINUI3_SWAPCHAINPANELNATIVEWINDOW_H_

#include "libANGLE/renderer/d3d/d3d11/winui3/InspectableNativeWindow.h"

#include <memory>

namespace rx
{
class SwapChainPanelNativeWindow : public InspectableNativeWindow,
                                   public std::enable_shared_from_this<SwapChainPanelNativeWindow>
{
  public:
    ~SwapChainPanelNativeWindow();

    bool initialize(
        EGLNativeWindowType window,
        const winrt::Windows::Foundation::Collections::IPropertySet &propertySet) override;
    HRESULT createSwapChain(ID3D11Device *device,
                            IDXGIFactory2 *factory,
                            DXGI_FORMAT format,
                            unsigned int width,
                            unsigned int height,
                            bool containsAlpha,
                            IDXGISwapChain1 **swapChain) override;

  protected:
    HRESULT scaleSwapChain(const winrt::Windows::Foundation::Size &windowSize,
                           const RECT &clientRect) override;

    bool registerForSizeChangeEvents();
    void unregisterForSizeChangeEvents();

  private:
    winrt::Microsoft::UI::Xaml::Controls::SwapChainPanel mSwapChainPanel;
    winrt::Windows::Foundation::Collections::IPropertySet mPropertySet;
    winrt::com_ptr<IDXGISwapChain1> mSwapChain;
};

HRESULT GetSwapChainPanelSize(
    winrt::Microsoft::UI::Xaml::Controls::SwapChainPanel &swapChainPanel,
                              winrt::Windows::Foundation::Size *windowSize);

}  // namespace rx
#endif  // LIBANGLE_RENDERER_D3D_D3D11_WINUI3_SWAPCHAINPANELNATIVEWINDOW_H_
