#pragma once

#include <string>
#include <vector>

namespace CloudSync
{
	// Locates the rclone executable on this system. Returns an empty string if not found.
	std::string FindRclonePath();

#if !BOOST_OS_WINDOWS
	// Builds a copy of the current environment with AppImage-specific variables stripped
	// (LD_LIBRARY_PATH/LD_PRELOAD/APPDIR/APPIMAGE/OWD/ARGV0), so a spawned rclone process
	// resolves its own shared library dependencies from the host system instead of picking up
	// Cemu's AppImage-bundled ones. Used by any code that posix_spawns rclone directly.
	std::vector<std::string> BuildCleanEnv();
#endif

	// Runs `rclone lsd <remoteName>:Cemu Cloud Saves` to verify the given rclone remote is
	// configured and reachable. outMessage is filled with either a success summary (rclone path
	// + synced game list) or, on failure, an explanation and setup instructions for this platform.
	// Returns true if the check succeeded.
	bool CheckCloudRemote(const std::string& remoteName, std::string& outMessage);
}
