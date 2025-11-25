# include "CGIProcess.hpp"


CGIProcess::CGIProcess() :
					_state(CGI_READING_DATA),
					_client_fd(-1),
					_pid(-1),
					_pipe_to_cgi(-1),
					_pipe_from_cgi(-1),
					_output_buffer(""),
					_bytes_written(0),
					_start_time(std::time(0))
					{}

CGIProcess::CGIProcess(int fd, int pid, int pipe_to_cgi, int pipe_from_cgi) :
					_state(CGI_READING_DATA),
					_client_fd(fd),
					_pid(pid),
					_pipe_to_cgi(pipe_to_cgi),
					_pipe_from_cgi(pipe_from_cgi),
					_output_buffer(""),
					_bytes_written(0),
					_start_time(std::time(0))
					{}

CGIProcess::CGIProcess(const CGIProcess &other) :
					_state(other._state),
					_client_fd(other._client_fd),
					_pid(other._pid),
					_pipe_to_cgi(other._pipe_to_cgi),
					_pipe_from_cgi(other._pipe_from_cgi),
					_output_buffer(other._output_buffer),
					_bytes_written(other._bytes_written),
					_start_time(other._start_time)
					{}

CGIProcess &CGIProcess::operator=(const CGIProcess &other)
{
	if (this != &other)
	{
		_state = other._state;
		_client_fd = other._client_fd;
		_pid = other._pid;
		_pipe_to_cgi = other._pipe_to_cgi;
		_pipe_from_cgi = other._pipe_from_cgi;
		_output_buffer = other._output_buffer;
		_bytes_written = other._bytes_written;
		_start_time = other._start_time;
	}
	return *this;
}

CGIProcess::~CGIProcess() {}
