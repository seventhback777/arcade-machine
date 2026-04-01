#include "Process.h"
#include <string>
#include <unistd.h>
#include <iostream>
#include <array>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <sys/wait.h>
#include <cerrno>
#include <cstring>

pid_t spawnProcess(std::string directory, std::string fileName) {

#ifndef _WIN32

	// First, fork the current process into a new process.
	// This is required to ensure that process execution occurs concurrently.
	pid_t processId = fork();

	if (processId == -1) {
		std::cerr << "[Process] fork failed: " << strerror(errno) << std::endl;
		return -1;
	}

	if (processId > 0) {
		std::cerr << "[Process] spawned game: " << directory << "/" << fileName << " (PID: " << processId << ")" << std::endl;
		return processId;
	}

    // Create new process group so we can kill the entire game tree later
    setpgid(0, 0);

    std::array<char, 128> buffer;

	// The working directory must be changed to the root directory of the game
	// to ensure that SplashKit resources are correctly pathed when the process
	// executes.
	if (chdir(directory.c_str()) != 0) {
		std::cerr << "[Process] chdir failed: " << directory << std::endl;
		exit(EXIT_FAILURE);
	}

	std::string exePath = "./builds/" + fileName;
	chmod(exePath.c_str(), 0755);
	std::string cmd = exePath + " 2>&1";
	auto pipe = popen(cmd.c_str(), "r");
	if (! pipe) {
		std::cerr << "[Process] popen failed: " << cmd << std::endl;
		exit(EXIT_FAILURE);
	}

	// At this point, we're doing nothing with the stdout and stderr
	// streams. This could be extended by piping them into shared
	// memory for consumption by the main process.
	while (fgets(buffer.data(), 128, pipe) != NULL) {
		std::cerr << "[Game] " << buffer.data();
	}

	int ret = pclose(pipe);
	int gameExitCode = WIFEXITED(ret) ? WEXITSTATUS(ret) : 1;

	exit(gameExitCode);

#endif

	// TODO: Add Windows support.
	return 0;

}

bool processRunning(pid_t processId, int &exitCode) {

#ifndef _WIN32
	int status;
	pid_t result = waitpid(processId, &status, WNOHANG);
	if (result == 0) return true;   // still running
	if (result == processId) {
		if (WIFEXITED(status))
			exitCode = WEXITSTATUS(status);
		else if (WIFSIGNALED(status))
			exitCode = 128 + WTERMSIG(status);  // signal = non-zero = crash
		else
			exitCode = -1;
	}
	return false;
#endif

	// TODO: Add Windows support.
	exitCode = 0;
	return false;

}

void killProcess(pid_t processId) {
#ifndef _WIN32
	std::cerr << "[Process] killing game process group (PID: " << processId << ")" << std::endl;
	kill(-processId, SIGTERM);
#endif
}
