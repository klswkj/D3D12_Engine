#pragma once

// HRESULT hr should exist in the local scope for these macros to work

#define GFX_EXCEPT_NOINFO(hardwareResult) Graphics::HrException( __LINE__,__FILE__,(hardwareResult) )
#define GFX_THROW_NOINFO(hrcall) if( FAILED( hardwareResult = (hrcall) ) ) throw Graphics::HrException( __LINE__,__FILE__,hardwareResult )

#ifndef NDEBUG
#define GFX_EXCEPT(hardwareResult) Graphics::HrException( __LINE__,__FILE__,(hardwareResult),infoManager.GetMessages() )
#define GFX_THROW_INFO(hrcall) infoManager.Set(); if( FAILED( hardwareResult = (hrcall) ) ) throw GFX_EXCEPT(hardwareResult)
#define GFX_DEVICE_REMOVED_EXCEPT(hardwareResult) Graphics::DeviceRemovedException( __LINE__,__FILE__,(hardwareResult),infoManager.GetMessages() )
#define GFX_THROW_INFO_ONLY(call) infoManager.Set(); (call); {auto v = infoManager.GetMessages(); if(!v.empty()) {throw Graphics::InfoException( __LINE__,__FILE__,v);}}
#else
#define GFX_EXCEPT(hardwareResult) Graphics::HrException( __LINE__,__FILE__,(hardwareResult) )
#define GFX_THROW_INFO(hrcall) GFX_THROW_NOINFO(hrcall)
#define GFX_DEVICE_REMOVED_EXCEPT(hardwareResult) Graphics::DeviceRemovedException( __LINE__,__FILE__,(hardwareResult) )
#define GFX_THROW_INFO_ONLY(call) (call)
#endif

// macro for importing infomanager into local scope
// this.GetInfoManager(Graphics& gfx) must exist
#ifdef NDEBUG
#define INFOMAN(gfx) HRESULT hardwareResult
#else
#define INFOMAN(gfx) HRESULT hardwareResult; DxgiInfoManager& infoManager = GetInfoManager((gfx))
#endif

#ifdef NDEBUG
#define INFOMAN_NOHR(gfx)
#else
#define INFOMAN_NOHR(gfx) DxgiInfoManager& infoManager = GetInfoManager((gfx))
#endif