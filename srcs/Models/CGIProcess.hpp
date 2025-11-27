#ifndef CGIPROCESS_H
# define CGIPROCESS_H

# include <vector>
# include <string>
# include <ctime>
# include <stdexcept>
# include <unistd.h>
# include <iostream>

class CGIProcess
{
	private:
		enum State {
			CGI_READING_DATA,
			CGI_PROCESSING_AND_WRITING_DATA,
			FINISH,
			HANDLE_ERROR
		};
		State _state;

		int _client_fd;
		pid_t _pid;
		int _pipe_to_cgi;    // Write end (stdin for child)
		int _pipe_from_cgi;  // Read end (stdout from child)
		std::string _output_buffer;
		int _bytes_written;
		time_t _start_time;  // For timeout

	public:
		CGIProcess();
		CGIProcess(const CGIProcess& other);
		CGIProcess& operator=(const CGIProcess& other);
		~CGIProcess();
		CGIProcess(int fd, int pid, int pipe_to_cgi, int pipe_from_cgi);


		void set_reading() { _state = CGI_READING_DATA; };
		void set_processing_and_writing() { _state = CGI_PROCESSING_AND_WRITING_DATA; };
		void set_finish() { _state = FINISH; };
		bool can_i_read() const { return _state == CGI_READING_DATA; };
		bool can_i_process_and_write() const { return _state == CGI_PROCESSING_AND_WRITING_DATA; };
		bool am_i_finish() const { return _state == FINISH; };

		void update_bytes_written(int res) { _bytes_written += res; };
		void append_to_output(const std::string& buf, int n) { _output_buffer.append(buf, n); };

		int get_client_fd() const { return _client_fd; };
		pid_t get_pid() const { return _pid; };
		int get_pipe_to_cgi() const { return _pipe_to_cgi; };
		int get_pipe_from_cgi() const { return _pipe_from_cgi; };
		int get_bytes_written() const { return _bytes_written; };
		std::string get_output_buffer() const { return _output_buffer; };

};

#endif
