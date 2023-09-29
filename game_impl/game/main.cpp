Arat:
#include "DragonSoul.h"

Altına ekle:
#ifdef ENABLE_NOMERCY
#include "NoMercy.h"
#endif

----

Arat:
	if (!g_bAuthServer)
	{

Altına ekle:
#ifdef ENABLE_NOMERCY
		if (!NoMercyInitialize(NOMERCY_LICENSE_CODE))
		{
			fprintf(stderr, "Failed To Initialize NoMercy");
			CleanUpForEarlyExit();
			return 0;
		}
#endif

----

Arat:
	building_manager.Destroy();

	if (!g_bAuthServer)
	{

Altına ekle:
#ifdef ENABLE_NOMERCY
		NoMercyUnloadServerPlugin();
#endif
