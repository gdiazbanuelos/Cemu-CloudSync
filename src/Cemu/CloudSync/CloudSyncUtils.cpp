#include "Cemu/CloudSync/CloudSyncUtils.h"

#include <fmt/format.h>
#include <boost/nowide/convert.hpp>
#include <vector>

#if BOOST_OS_WINDOWS
#include <Windows.h>
#else
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>
#include <cstdlib>
extern char** environ;
#endif

namespace CloudSync
{
#if BOOST_OS_WINDOWS
	std::string FindRclonePath()
	{
		wchar_t buffer[MAX_PATH];
		DWORD result = SearchPathW(nullptr, L"rclone.exe", nullptr, MAX_PATH, buffer, nullptr);
		if (result == 0 || result >= MAX_PATH)
			return {};
		return boost::nowide::narrow(buffer);
	}

	// Runs the given command line and captures combined stdout+stderr. Returns false if the
	// process could not be launched at all (outExitCode is only meaningful when this is true).
	static bool RunCaptured(const std::wstring& cmdLine, std::string& outOutput, sint32& outExitCode)
	{
		SECURITY_ATTRIBUTES sa{};
		sa.nLength = sizeof(sa);
		sa.bInheritHandle = TRUE;

		HANDLE readPipe, writePipe;
		if (!CreatePipe(&readPipe, &writePipe, &sa, 0))
			return false;
		SetHandleInformation(readPipe, HANDLE_FLAG_INHERIT, 0);

		STARTUPINFOW si{};
		si.cb = sizeof(si);
		si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
		si.wShowWindow = SW_HIDE;
		si.hStdOutput = writePipe;
		si.hStdError = writePipe;
		PROCESS_INFORMATION pi{};

		std::wstring mutableCmdLine = cmdLine;
		mutableCmdLine.push_back(L'\0');

		if (!CreateProcessW(nullptr, mutableCmdLine.data(), nullptr, nullptr, TRUE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi))
		{
			CloseHandle(readPipe);
			CloseHandle(writePipe);
			return false;
		}
		CloseHandle(writePipe);

		char buf[4096];
		DWORD bytesRead;
		while (ReadFile(readPipe, buf, sizeof(buf) - 1, &bytesRead, nullptr) && bytesRead > 0)
		{
			buf[bytesRead] = '\0';
			outOutput += buf;
		}
		CloseHandle(readPipe);

		WaitForSingleObject(pi.hProcess, INFINITE);
		DWORD exitCode = 0;
		GetExitCodeProcess(pi.hProcess, &exitCode);
		outExitCode = (sint32)exitCode;
		CloseHandle(pi.hThread);
		CloseHandle(pi.hProcess);
		return true;
	}
#else
	// Mirrors the lookup used by the save-sync code in nn_save.cpp: probe common install
	// locations directly instead of relying on $PATH, plus a /proc/1/root fallback so this
	// works when running from an AppImage's mount namespace.
	std::string FindRclonePath()
	{
		std::vector<std::string> candidates = {
			"/usr/bin/rclone",
			"/usr/local/bin/rclone",
			"/var/lib/flatpak/exports/bin/rclone",
		};
		if (const char* home = getenv("HOME"))
		{
			candidates.emplace_back(std::string(home) + "/bin/rclone");
			candidates.emplace_back(std::string(home) + "/.local/bin/rclone");
		}

		const std::vector<std::string> baseCandidates = candidates;
		for (const std::string& path : baseCandidates)
			candidates.push_back("/proc/1/root" + path);

		for (const std::string& path : candidates)
		{
			if (access(path.c_str(), X_OK) == 0)
				return path;
		}
		return {};
	}

	// AppImages may prepend their bundled lib dir to LD_LIBRARY_PATH; if that leaks into
	// rclone's process it can load Cemu-bundled versions of shared libs (e.g. libssl/libcrypto)
	// that don't match what rclone was linked/tested against, causing TLS/certificate failures
	// when talking to Dropbox. Mirrors CloudSync_BuildCleanEnv() in nn_save.cpp.
	static std::vector<std::string> BuildCleanEnv()
	{
		static const std::vector<std::string> blockedVars = {
			"LD_LIBRARY_PATH", "LD_PRELOAD", "APPDIR", "APPIMAGE", "OWD", "ARGV0"
		};

		std::vector<std::string> env;
		for (char** envp = environ; *envp != nullptr; ++envp)
		{
			std::string entry(*envp);
			const size_t eq = entry.find('=');
			const std::string key = (eq == std::string::npos) ? entry : entry.substr(0, eq);

			bool blocked = false;
			for (const std::string& blockedVar : blockedVars)
			{
				if (key == blockedVar)
				{
					blocked = true;
					break;
				}
			}
			if (!blocked)
				env.push_back(std::move(entry));
		}
		return env;
	}

	static bool RunCaptured(const std::string& rclonePath, const std::vector<std::string>& args, std::string& outOutput, sint32& outExitCode)
	{
		int pipefd[2];
		if (pipe(pipefd) != 0)
			return false;

		posix_spawn_file_actions_t actions;
		posix_spawn_file_actions_init(&actions);
		posix_spawn_file_actions_adddup2(&actions, pipefd[1], STDOUT_FILENO);
		posix_spawn_file_actions_adddup2(&actions, pipefd[1], STDERR_FILENO);
		posix_spawn_file_actions_addclose(&actions, pipefd[0]);
		posix_spawn_file_actions_addclose(&actions, pipefd[1]);

		std::vector<char*> argv;
		argv.push_back(const_cast<char*>(rclonePath.c_str()));
		for (const std::string& arg : args)
			argv.push_back(const_cast<char*>(arg.c_str()));
		argv.push_back(nullptr);

		std::vector<std::string> envStorage = BuildCleanEnv();
		std::vector<char*> envp;
		for (const std::string& entry : envStorage)
			envp.push_back(const_cast<char*>(entry.c_str()));
		envp.push_back(nullptr);

		pid_t pid;
		int spawnResult = posix_spawn(&pid, rclonePath.c_str(), &actions, nullptr, argv.data(), envp.data());
		posix_spawn_file_actions_destroy(&actions);
		close(pipefd[1]);

		if (spawnResult != 0)
		{
			close(pipefd[0]);
			return false;
		}

		char buf[4096];
		ssize_t bytesRead;
		while ((bytesRead = read(pipefd[0], buf, sizeof(buf) - 1)) > 0)
		{
			buf[bytesRead] = '\0';
			outOutput += buf;
		}
		close(pipefd[0]);

		int status = 0;
		waitpid(pid, &status, 0);
		outExitCode = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
		return true;
	}
#endif

	static std::string GetInstallInstructions()
	{
#if BOOST_OS_WINDOWS
		return
			"rclone was not found.\n\n"
			"To set it up:\n"
			"1. Download rclone from https://rclone.org/downloads/\n"
			"2. Extract it to a folder (e.g. C:\\rclone) and add that folder to your PATH\n"
			"3. Open a new terminal and run: rclone config\n"
			"4. Create a new remote named exactly \"Dropbox\" (type: dropbox), use auto config to authorize via browser\n"
			"5. Restart Cemu and re-open this tab to verify";
#else
		return
			"rclone was not found.\n\n"
			"To set it up:\n"
			"1. Install rclone to your user directory, e.g.:\n"
			"   mkdir -p ~/.local/bin\n"
			"   curl -O https://downloads.rclone.org/rclone-current-linux-amd64.zip\n"
			"   unzip rclone-current-linux-amd64.zip && cp rclone-*-linux-amd64/rclone ~/.local/bin/\n"
			"   chmod +x ~/.local/bin/rclone\n"
			"2. Run: ~/.local/bin/rclone config\n"
			"3. Create a new remote named exactly \"Dropbox\" (type: dropbox), use auto config to authorize via browser\n"
			"4. Restart Cemu and re-open this tab to verify";
#endif
	}

	bool CheckDropboxRemote(std::string& outMessage)
	{
		std::string rclonePath = FindRclonePath();
		if (rclonePath.empty())
		{
			outMessage = GetInstallInstructions();
			return false;
		}

		std::string output;
		sint32 exitCode = -1;
		bool spawnOk;
#if BOOST_OS_WINDOWS
		std::wstring cmdLine = L"\"" + boost::nowide::widen(rclonePath) + L"\" lsd \"Dropbox:Cemu Cloud Saves\"";
		spawnOk = RunCaptured(cmdLine, output, exitCode);
#else
		spawnOk = RunCaptured(rclonePath, {"lsd", "Dropbox:Cemu Cloud Saves"}, output, exitCode);
#endif

		if (!spawnOk)
		{
			outMessage = fmt::format("rclone found at:\n{}\n\nBut it could not be launched.", rclonePath);
			return false;
		}

		if (exitCode == 0)
		{
			outMessage = fmt::format("rclone found at:\n{}\n\nDropbox remote is configured correctly.\n\nGames with synced saves:\n{}", rclonePath, output.empty() ? "(none yet)" : output);
			return true;
		}

		// The "Cemu Cloud Saves" folder doesn't exist yet if no save has ever been pushed to
		// Dropbox on this remote. That's expected on a fresh setup, not a real failure -
		// CloudSync creates the folder automatically the first time a game saves.
		if (output.find("directory not found") != std::string::npos)
		{
			outMessage = fmt::format("rclone found at:\n{}\n\nDropbox remote is configured correctly.\n\nNo \"Cemu Cloud Saves\" folder yet - it will be created automatically the first time a game pushes a save.", rclonePath);
			return true;
		}

		outMessage = fmt::format(
			"rclone found at:\n{}\n\nBut \"rclone lsd Dropbox:Cemu Cloud Saves\" failed (exit code {}):\n{}\n\n"
			"Make sure you've configured a remote named exactly \"Dropbox\" by running: rclone config",
			rclonePath, exitCode, output.empty() ? "(no output)" : output);
		return false;
	}
}
