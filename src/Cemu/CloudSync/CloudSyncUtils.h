#pragma once

#include <string>

namespace CloudSync
{
	// Locates the rclone executable on this system. Returns an empty string if not found.
	std::string FindRclonePath();

	// Runs `rclone lsd <remoteName>:Cemu Cloud Saves` to verify the given rclone remote is
	// configured and reachable. outMessage is filled with either a success summary (rclone path
	// + synced game list) or, on failure, an explanation and setup instructions for this platform.
	// Returns true if the check succeeded.
	bool CheckCloudRemote(const std::string& remoteName, std::string& outMessage);
}
