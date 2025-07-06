#define CINTERFACE
#define D3D11_NO_HELPERS
#define INITGUID

#include "dxgi1_2.h"
#include "helpers.h"
#include "config.h"
#include "bnusio.h"
#include "patches.h"

/*
 * Reference: https://github.com/teknogods/OpenParrot/blob/master/OpenParrot/src/Functions/WindowedDxgi.cpp
 */

namespace patches::Dxgi {

// Local variables
static bool FpsLimiterEnable = false;

// Prototypes
static HRESULT (STDMETHODCALLTYPE *g_oldPresentWrap) (IDXGISwapChain *pSwapChain, UINT SyncInterval, UINT Flags);
static HRESULT (STDMETHODCALLTYPE *g_oldCreateSwapChain2) (IDXGIFactory2 *This, IUnknown *pDevice, DXGI_SWAP_CHAIN_DESC *pDesc,
                                                           IDXGISwapChain **ppSwapChain);
static HRESULT (WINAPI *g_origCreateDXGIFactory2) (UINT Flags, REFIID riid, void **ppFactory);

static HRESULT STDMETHODCALLTYPE PresentWrap (IDXGISwapChain *pSwapChain, UINT SyncInterval, UINT Flags);
static HRESULT STDMETHODCALLTYPE CreateSwapChain2Wrap (IDXGIFactory2 *This, IUnknown *pDevice, DXGI_SWAP_CHAIN_DESC *pDesc,
                                                       IDXGISwapChain **ppSwapChain);
static HRESULT WINAPI CreateDXGIFactory2Wrap (UINT Flags, REFIID riid, void **ppFactory);

// Functions
template <typename T>
T
HookVtableFunction (T *functionPtr, T target) {
    if (*functionPtr == target) return nullptr;

    auto old = *functionPtr;
    WRITE_MEMORY (functionPtr, T, target);

    return old;
}

static HRESULT STDMETHODCALLTYPE
PresentWrap (IDXGISwapChain *pSwapChain, const UINT SyncInterval, const UINT Flags) {
    if (FpsLimiterEnable) FpsLimiter::Update ();

    bnusio::Update ();

    return g_oldPresentWrap (pSwapChain, SyncInterval, Flags);
}

static HRESULT STDMETHODCALLTYPE
CreateSwapChain2Wrap (IDXGIFactory2 *This, IUnknown *pDevice, DXGI_SWAP_CHAIN_DESC *pDesc, IDXGISwapChain **ppSwapChain) {
    const HRESULT hr = g_oldCreateSwapChain2 (This, pDevice, pDesc, ppSwapChain);

    if (*ppSwapChain) {
        if (FpsLimiterEnable) {
            const auto old2  = HookVtableFunction (&(*ppSwapChain)->lpVtbl->Present, PresentWrap);
            g_oldPresentWrap = old2 ? old2 : g_oldPresentWrap;
        }
    }

    return hr;
}

static HRESULT WINAPI
CreateDXGIFactory2Wrap (const UINT Flags, REFIID riid, void **ppFactory) {
    const HRESULT hr = g_origCreateDXGIFactory2 (Flags, riid, ppFactory);

    if (SUCCEEDED (hr)) {
        const IDXGIFactory2 *factory = static_cast<IDXGIFactory2 *> (*ppFactory);

        const auto old        = HookVtableFunction (&factory->lpVtbl->CreateSwapChain, CreateSwapChain2Wrap);
        g_oldCreateSwapChain2 = old ? old : g_oldCreateSwapChain2;
    }

    return hr;
}

void
Init () {
    LogMessage (LogLevel::INFO, "Init DXGI patches");
    FpsLimiterEnable = Config::ConfigManager::instance ().getGraphicsConfig ().fpslimit;

    if (FpsLimiterEnable) {
        FpsLimiter::Init ();

        MH_Initialize ();
        MH_CreateHookApi (L"dxgi.dll", "CreateDXGIFactory2", reinterpret_cast<LPVOID> (CreateDXGIFactory2Wrap),
                        reinterpret_cast<void **> (&g_origCreateDXGIFactory2));
        MH_EnableHook (nullptr);
    }
}

} // namespace patches::Dxgi