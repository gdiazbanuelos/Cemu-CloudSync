#pragma once

#include <string>

namespace CloudSync
{
	// Locates the rclone executable on this system. Returns an empty string if not found.
	std::string FindRclonePath();

	// Runs `rclone lsd Dropbox:` to verify the "Dropbox" remote is configured and reachable.
	// outMessage is filled with either a success summary (rclone path + remote listing) or,
	// on failure, an explanation and setup instructions for this platform.
	// Returns true if the check succeeded.
	bool CheckDropboxRemote(std::string& outMessage);
}
