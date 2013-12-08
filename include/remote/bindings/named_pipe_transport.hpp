// Copyright 2013 Ng Kwan Ti <ngkwanti -at- gmail.com>
//
// This file is distributed under GPL v2 license. You can redistribute it and/or
// modify it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation. See <http://www.gnu.org/licenses/>.
//
// See www.cppremote.com for documentation.
//-----------------------------------------------------------------------------
#ifndef __REMOTE_BINDINGS_NAMED_PIPE_TRANSPORT_HPP__
#define __REMOTE_BINDINGS_NAMED_PIPE_TRANSPORT_HPP__

#include <remote/bindings/fwd.hpp>
#include <remote/bindings/named_pipe_endpoint.hpp>
#ifdef BOOST_ASIO_HAS_WINDOWS_STREAM_HANDLE

#include <remote/bindings/named_pipe_acceptor.hpp>
#include <remote/bindings/buffer.hpp>

#include <boost/asio/strand.hpp>
#include <boost/noncopyable.hpp>
#include <boost/function.hpp>

#include <deque>


namespace remote
{
namespace bindings
{

class named_pipe_transport : public boost::noncopyable
{
public:
	enum state_t {stopped, starting, started, stopping};

	typedef named_pipe_acceptor acceptor_type;
	typedef named_pipe_endpoint endpoint_type;

	typedef boost::function<void(exception_ptr)> handler;
	typedef boost::function<void(exception_ptr, buffer_ptr)> read_handler;

	static void default_handler(exception_ptr){}

public:
	explicit named_pipe_transport(io_service& ios);
	~named_pipe_transport();

	state_t state() const { return m_state; }

	void accept(acceptor_type& ac, handler const& hdl);
	void connect(endpoint_type const& ep, handler const& hdl);
	void disconnect(handler const& hdl);

	void write(buffer_ptr const& buf, handler const& hdl);
	void read(read_handler const& hdl);

private:
	typedef named_pipe_transport this_type;

	// do we need to specify packing of header structure?
	struct header
	{
		enum { magic = 0x3a29 };

		boost::uint16_t m_magic;
		boost::uint16_t m_reserve;
		boost::uint32_t m_size;

		header() {}
		header(boost::uint32_t size);

		bool validate();
	};

	struct write_operation
	{
		header m_header;
		buffer_ptr m_buffer;
		handler m_handler;

		write_operation(buffer_ptr const& buf, handler const& hdl)
		: m_header(static_cast<boost::uint32_t>(buf->size()))
		, m_buffer(buf)
		, m_handler(hdl)
		{}
	};

	typedef boost::system::error_code error_code;
	typedef boost::asio::windows::stream_handle stream_handle;
	typedef std::deque<write_operation> write_operations;

private:
	state_t m_state;
	io_service& m_io_service;
	mutable io_service::strand m_strand;
	stream_handle m_pipe;

	write_operations m_write_operations;
	size_t m_write_count;

	buffer_ptr m_read_buffer;
	read_handler m_read_handler;

private:
	void do_accept(acceptor_type& ac, handler hdl);
	void on_accept(error_code ec, handler hdl);

	void do_connect(endpoint_type ep, handler hdl);
	void on_connect(error_code ec, HANDLE pipe_handle, handler hdl);

	void signal_disconnect(handler hdl);
	void do_disconnect(error_code ec, handler hdl);

	void start_read(read_handler hdl);
	void do_read();
	void on_read_size(error_code ec, std::size_t transfered);
	void on_read_data(error_code ec, std::size_t transfered);

	void queue_write(buffer_ptr buf, handler hdl);
	void do_write();
	void on_write(error_code ec, std::size_t tx);
};

}
}

#endif
#endif
