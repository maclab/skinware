#ifndef SKIN_USER_VERSION_H
#define SKIN_USER_VERSION_H

#define SKIN_STRINGIFY_(x)	#x
#define SKIN_STRINGIFY(x)	SKIN_STRINGIFY_(x)
#define SKIN_VERSION_MAJOR	@SKIN_VERSION_MAJOR@
#define SKIN_VERSION_MINOR	@SKIN_VERSION_MINOR@
#define SKIN_VERSION_REVISION	@SKIN_VERSION_REVISION@
#define SKIN_VERSION_BUILD	@SKIN_VERSION_BUILD@
#define SKIN_VERSION		SKIN_STRINGIFY(SKIN_VERSION_MAJOR)"."\
				SKIN_STRINGIFY(SKIN_VERSION_MINOR)"."\
				SKIN_STRINGIFY(SKIN_VERSION_REVISION)"."\
				SKIN_STRINGIFY(SKIN_VERSION_BUILD)

#endif
