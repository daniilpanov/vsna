#include "node_api.h"

#include <stdexcept>

#include "addr.h"
#include "config.h"
#include "node.h"

NodeApi::NodeApi() : _node(std::make_shared<Node>())
{}

NodeApi::~NodeApi()
{
	if (_node)
		_node->stop();
}

void NodeApi::configure(const std::string& ip, const std::string& port, const std::string& path)
{
	_node->setConfig(Config(Addr(ip, port), path));
}

void NodeApi::configureFromFile(const std::string& configFile)
{
	_node->setConfig(Config::loadFromFile(configFile));
}

std::string NodeApi::describe() const
{
	return _node->getConfig().toString();
}

std::string NodeApi::path() const
{
	return _node->getConfig().getPath();
}

void NodeApi::start()
{
	_node->start();
}

void NodeApi::stop()
{
	_node->stop();
}

void NodeApi::connect(const std::string& host, const std::string& port)
{
	_node->connect(host, port);
}

void NodeApi::addPeer(const std::string& host, const std::string& port)
{
	_node->addPeer(host, port);
}

void NodeApi::connectToAllKnown()
{
	_node->connectToAllKnown();
}

std::vector<std::string> NodeApi::knownPeers() const
{
	return _node->peers().known();
}

std::vector<std::string> NodeApi::connectedPeers() const
{
	return _node->peers().connected();
}

void NodeApi::sendFile(const std::string& peerAddr, const std::string& localPath)
{
	_node->sendFile(peerAddr, localPath);
}

void NodeApi::setOnPeerDiscovered(std::function<void(const std::string&)> obs)
{
	_node->setOnPeerDiscovered(std::move(obs));
}