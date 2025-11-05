#ifndef DISPATCHER_HPP
# define DISPATCHER_HPP

//webserv
#include <init/ClientConnection.hpp>
#include <config/LocationConfig.hpp>

class Dispatcher
{
	private:
		Dispatcher(); //blocked
		~Dispatcher(); //blocked
		Dispatcher(const Dispatcher& rhs); //blocked
		Dispatcher& operator=(const Dispatcher& rhs); //blocked

	public:
		static void	dispatch(ClientConnection& client);
};

#endif //DISPATCHER_HPP