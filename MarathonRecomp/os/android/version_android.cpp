#include <os/version.h>

#include <sys/system_properties.h>

os::version::OSVersion os::version::GetOSVersion()
{
    os::version::OSVersion version{};

    char release[PROP_VALUE_MAX] = {};
    if (__system_property_get("ro.build.version.release", release) > 0)
    {
        sscanf(release, "%u.%u.%u", &version.Major, &version.Minor, &version.Build);
    }

    return version;
}
