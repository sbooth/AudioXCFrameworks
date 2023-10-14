#pragma warning(disable: 4464) // includes with .. but the Winamp code does this now, so let it pass here
#pragma warning(disable: 4574) // another Winamp warning
#include "../stdafx.h"
#include "api/service/api_service.h"
#include "Agave/Config/api_config.h"
#include "api/memmgr/api_memmgr.h"
#include "api/service/waservicefactory.h"
#include "Winamp/IN2.H"
#include "Winamp/wa_ipc.h"
#include "api/service/waservicefactory.h"
#include "Agave/AlbumArt/svc_albumArtProvider.h"
#include "Wasabi.h"

extern In_Module g_APEWinampPluginModule;

class AlbumArtFactory : public waServiceFactory
{
public:
    FOURCC GetServiceType();
    const char *GetServiceName();
    GUID GetGUID();
    void *GetInterface(int global_lock);
    int SupportNonLockingInterface();
    int ReleaseInterface(void *ifc);
    const char *GetTestString();
    int ServiceNotify(int msg, int param1, int param2);

protected:
    RECVS_DISPATCH;
};

//extern "C" extern In_Module mod; // TODO: change if you called yours something else
#define WASABI_API_MEMMGR memmgr

static api_config *AGAVE_API_CONFIG = 0;
static api_service *WASABI_API_SVC = 0;
static api_memmgr *WASABI_API_MEMMGR = 0;

static AlbumArtFactory albumArtFactory;
void Wasabi_Init()
{
    WASABI_API_SVC = reinterpret_cast<api_service *>(SendMessage(g_APEWinampPluginModule.hMainWindow, WM_WA_IPC, 0, IPC_GET_API_SERVICE));

    if (WASABI_API_SVC == 0 || WASABI_API_SVC == reinterpret_cast<api_service *>(1))
    {
        WASABI_API_SVC = 0;
        return;
    }

    WASABI_API_SVC->service_register(&albumArtFactory);

    waServiceFactory * sf = reinterpret_cast<waServiceFactory *>(WASABI_API_SVC->service_getServiceByGuid(AgaveConfigGUID));
    if (sf)
        AGAVE_API_CONFIG = reinterpret_cast<api_config *>(sf->getInterface());
    sf = reinterpret_cast<waServiceFactory *>(WASABI_API_SVC->service_getServiceByGuid(memMgrApiServiceGuid));
    if (sf)
        WASABI_API_MEMMGR = reinterpret_cast<api_memmgr *>(sf->getInterface());
}

void Wasabi_Quit()
{
    if (WASABI_API_SVC)
    {
        waServiceFactory *sf = reinterpret_cast<waServiceFactory *>(WASABI_API_SVC->service_getServiceByGuid(AgaveConfigGUID));
        if (sf)
            sf->releaseInterface(AGAVE_API_CONFIG);
        sf = reinterpret_cast<waServiceFactory *>(WASABI_API_SVC->service_getServiceByGuid(memMgrApiServiceGuid));
        if (sf)
            sf->releaseInterface(WASABI_API_MEMMGR);

        WASABI_API_SVC->service_deregister(&albumArtFactory);
    }
}

void * Wasabi_Malloc(size_t size_in_bytes)
{
    return WASABI_API_MEMMGR->sysMalloc(size_in_bytes);
}

void Wasabi_Free(void *memory_block)
{
    WASABI_API_MEMMGR->sysFree(memory_block);
}

class APE_AlbumArtProvider : public svc_albumArtProvider
{
public:
    bool IsMine(const wchar_t *filename);
    int ProviderType();
    // implementation note: use WASABI_API_MEMMGR to alloc bits and mimetype, so that the recipient can free through that
    int GetAlbumArtData(const wchar_t *filename, const wchar_t *type, void **bits, size_t *len, wchar_t **mimeType);
    int SetAlbumArtData(const wchar_t *filename, const wchar_t *type, void *bits, size_t len, const wchar_t *mimeType);
    int DeleteAlbumArt(const wchar_t *filename, const wchar_t *type);
protected:
    RECVS_DISPATCH;
};

static const wchar_t *GetLastCharactercW(const wchar_t *string)
{
    if (!string || !*string)
        return string;

    return CharPrevW(string, string+lstrlenW(string));
}

static const wchar_t *scanstr_backW(const wchar_t *str, const wchar_t *toscan, const wchar_t *defval)
{
    const wchar_t *s = GetLastCharactercW(str);
    if (!str[0]) return defval;
    if (!toscan || !toscan[0]) return defval;
    while (1)
    {
        const wchar_t *t = toscan;
        while (*t)
        {
            if (*t == *s) return s;
            t = CharNextW(t);
        }
        t = CharPrevW(str, s);
        if (t == s)
            return defval;
        s = t;
    }
}

static const wchar_t *extensionW(const wchar_t *fn)
{
    const wchar_t *end = scanstr_backW(fn, L"./\\", 0);
    if (!end)
        return (fn+lstrlenW(fn));

    if (*end == L'.')
        return end+1;

    return (fn+lstrlenW(fn));
}

bool APE_AlbumArtProvider::IsMine(const wchar_t *filename)
{
    const wchar_t * extension = extensionW(filename);
    if (extension && *extension)
    {
        if ((_tcsicmp(extension, _T("ape")) == 0) ||
            (_tcsicmp(extension, _T("mac")) == 0) ||
            (_tcsicmp(extension, _T("apl")) == 0))
        {
            return true;
        }
    }
    return false;
}

int APE_AlbumArtProvider::ProviderType()
{
    return ALBUMARTPROVIDER_TYPE_EMBEDDED;
}

extern "C" int APE_GetAlbumArt(const wchar_t * filename, const wchar_t * type, void ** bits, size_t * len, wchar_t ** mime_type);
int APE_AlbumArtProvider::GetAlbumArtData(const wchar_t * filename, const wchar_t * type, void ** bits, size_t * len, wchar_t ** mime_type)
{
    return APE_GetAlbumArt(filename, type, bits, len, mime_type);
}

extern "C" int APE_SetAlbumArt(const wchar_t * filename, const wchar_t *type, void * bits, size_t len, const wchar_t *mime_type);
int APE_AlbumArtProvider::SetAlbumArtData(const wchar_t * filename, const wchar_t * type, void * bits, size_t len, const wchar_t * mime_type)
{
    return APE_SetAlbumArt(filename, type, bits, len, mime_type);
}

extern "C" int APE_DeleteAlbumArt(const wchar_t * filename, const wchar_t * type);
int APE_AlbumArtProvider::DeleteAlbumArt(const wchar_t * filename, const wchar_t * type)
{
    return APE_DeleteAlbumArt(filename, type);
}

#define CBCLASS APE_AlbumArtProvider
START_DISPATCH
CB(SVC_ALBUMARTPROVIDER_PROVIDERTYPE, ProviderType)
CB(SVC_ALBUMARTPROVIDER_GETALBUMARTDATA, GetAlbumArtData)
CB(SVC_ALBUMARTPROVIDER_ISMINE, IsMine)
CB(SVC_ALBUMARTPROVIDER_SETALBUMARTDATA, SetAlbumArtData)
CB(SVC_ALBUMARTPROVIDER_DELETEALBUMART, DeleteAlbumArt)
END_DISPATCH
#undef CBCLASS

static APE_AlbumArtProvider albumArtProvider;

// {1636EE69-E2D3-4C01-854F-B429394ACD7F}
static const GUID ape_albumartproviderGUID =
{ 0x1636ee69, 0xe2d3, 0x4c01, { 0x85, 0x4f, 0xb4, 0x29, 0x39, 0x4a, 0xcd, 0x7f } };

FOURCC AlbumArtFactory::GetServiceType()
{
    return svc_albumArtProvider::SERVICETYPE;
}

const char *AlbumArtFactory::GetServiceName()
{
    return "APE Album Art Provider";
}

GUID AlbumArtFactory::GetGUID()
{
    return ape_albumartproviderGUID;
}

void *AlbumArtFactory::GetInterface(int global_lock)
{
    return &albumArtProvider;
}

int AlbumArtFactory::SupportNonLockingInterface()
{
    return 1;
}

int AlbumArtFactory::ReleaseInterface(void *ifc)
{
    //WASABI_API_SVC->service_unlock(ifc);
    return 1;
}

const char *AlbumArtFactory::GetTestString()
{
    return 0;
}

int AlbumArtFactory::ServiceNotify(int msg, int param1, int param2)
{
    return 1;
}

#define CBCLASS AlbumArtFactory
START_DISPATCH
CB(WASERVICEFACTORY_GETSERVICETYPE, GetServiceType)
CB(WASERVICEFACTORY_GETSERVICENAME, GetServiceName)
CB(WASERVICEFACTORY_GETGUID, GetGUID)
CB(WASERVICEFACTORY_GETINTERFACE, GetInterface)
CB(WASERVICEFACTORY_SUPPORTNONLOCKINGGETINTERFACE, SupportNonLockingInterface)
CB(WASERVICEFACTORY_RELEASEINTERFACE, ReleaseInterface)
CB(WASERVICEFACTORY_GETTESTSTRING, GetTestString)
CB(WASERVICEFACTORY_SERVICENOTIFY, ServiceNotify)
END_DISPATCH
#undef CBCLASS
