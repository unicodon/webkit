//
// Copyright 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// InspectableNativeWindow.cpp: NativeWindow base class for managing IInspectable native window
// types.

#include "libANGLE/renderer/d3d/d3d11/winui3/SwapChainPanelNativeWindow.h"

namespace rx
{

bool IsSwapChainPanel(EGLNativeWindowType window, winrt::Microsoft::UI::Xaml::Controls::SwapChainPanel * swapChainPanel)
{
    if (!window)
    {
        return false;
    }

    winrt::com_ptr<::IInspectable> win;
    win.copy_from(window);

    if (swapChainPanel != nullptr)
    {
        return win.try_as<winrt::Microsoft::UI::Xaml::Controls::SwapChainPanel>(*swapChainPanel);
    }
    else
    {
        winrt::Microsoft::UI::Xaml::Controls::SwapChainPanel dummy;
        return win.try_as<winrt::Microsoft::UI::Xaml::Controls::SwapChainPanel>(dummy);
    }
}

bool IsEGLConfiguredPropertySet(EGLNativeWindowType window,
                                winrt::Windows::Foundation::Collections::IPropertySet *propertySet,
                                EGLNativeWindowType *eglNativeWindow)
{
    if (!window)
    {
        return false;
    }

    winrt::com_ptr<IInspectable> inspectable;
    inspectable.copy_from(window);

    winrt::Windows::Foundation::Collections::IPropertySet props;
    if (!inspectable.try_as(props))
    {
        return false;
    }

    // If the IPropertySet does not contain the required EglNativeWindowType key, the property set
    // is considered invalid.
    if (!props.HasKey(EGLNativeWindowTypeProperty))
    {
        ERR() << "Could not find EGLNativeWindowTypeProperty in IPropertySet. Valid "
                 "EGLNativeWindowTypeProperty values include ICoreWindow";
        return false;
    }

    // The EglNativeWindowType property exists, so retreive the IInspectable that represents the
    // EGLNativeWindowType
    winrt::com_ptr<IInspectable> nativeWindow;
    if (!props.Lookup(EGLNativeWindowTypeProperty).try_as(nativeWindow))
    {
        return false;
    }

    if (propertySet != nullptr)
    {
        *propertySet = props;
    }

    if (eglNativeWindow != nullptr)
    {
        nativeWindow.copy_to(eglNativeWindow);
    }

    return true;
}

// Retrieve an optional property from a property set
bool GetOptionalPropertyValue(
    const winrt::Windows::Foundation::Collections::IPropertySet &propertySet,
    const wchar_t *propertyName,
    bool &hasKey,
    winrt::Windows::Foundation::IInspectable &propertyValue)
{
    if (!propertySet)
    {
        return false;
    }

    propertyValue = propertySet.TryLookup(propertyName);
    hasKey        = !!propertyValue;
    
    return true;
}

// Attempts to read an optional SIZE property value that is assumed to be in the form of
// an ABI::Windows::Foundation::Size.  This function validates the Size value before returning
// it to the caller.
//
// Possible return values are:
// S_OK, valueExists == true - optional SIZE value was successfully retrieved and validated
// S_OK, valueExists == false - optional SIZE value was not found
// E_INVALIDARG, valueExists = false - optional SIZE value was malformed in the property set.
//    * Incorrect property type ( must be PropertyType_Size)
//    * Invalid property value (width/height must be > 0)
// Additional errors may be returned from IMap or IPropertyValue
//
bool GetOptionalSizePropertyValue(
    const winrt::Windows::Foundation::Collections::IPropertySet &propertySet,
    const wchar_t *propertyName,
    SIZE &value,
    bool &valueExists)
{
    valueExists = false;

    if (!propertySet)
    {
        return false;
    }

    winrt::Windows::Foundation::IInspectable propertyValue = propertySet.TryLookup(propertyName);
    if (!propertyValue)
    {
        // optional Size value was not found
        return true;
    }

    auto size = propertyValue.try_as<winrt::Windows::Foundation::Size>();
    if (!size)
    {
        // An invalid property type was detected. Size property must be of winrt::Windows::Foundation::Size
        return false;
    }

    if (size->Width <= 0 || size->Height <= 0)
    {
        // An invalid Size property was detected. Width/Height values must > 0
        return false;
    }

    // A valid property value exists
    valueExists = true;
    value.cx    = size->Width;
    value.cy    = size->Height;
    return true;
}

// Attempts to read an optional float property value that is assumed to be in the form of
// an ABI::Windows::Foundation::Single.  This function validates the Single value before returning
// it to the caller.
//
// Possible return values are:
// S_OK, valueExists == true - optional Single value was successfully retrieved and validated
// S_OK, valueExists == false - optional Single value was not found
// E_INVALIDARG, valueExists = false - optional Single value was malformed in the property set.
//    * Incorrect property type ( must be PropertyType_Single)
//    * Invalid property value (must be > 0)
// Additional errors may be returned from IMap or IPropertyValue
//
bool GetOptionalSinglePropertyValue(
    const winrt::Windows::Foundation::Collections::IPropertySet &propertySet,
    const wchar_t *propertyName,
    float &value,
    bool &valueExists)
{
    valueExists = false;

    if (!propertySet)
    {
        return false;
    }

    winrt::Windows::Foundation::IInspectable propertyValue = propertySet.TryLookup(propertyName);
    if (!propertyValue)
    {
        // optional Scale value was not found
        return true;
    }

    auto scale = propertyValue.try_as<float>();
    if (!scale)
    {
        // An invalid property type was detected. Size property must be of float
        return false;
    }

    if (*scale <= 0)
    {
        // An invalid Scale property was detected. Scale value must > 0
        return false;
    }

    // A valid property value exists
    valueExists = true;
    value       = *scale;
    return true;
}

RECT InspectableNativeWindow::clientRect(const winrt::Windows::Foundation::Size &size)
{
    // We don't have to check if a swapchain scale was specified here; the default value is 1.0f
    // which will have no effect.
    return {0, 0, lround(size.Width * mSwapChainScale), lround(size.Height * mSwapChainScale)};
}
}  // namespace rx
