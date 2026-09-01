#include "peer_registry.h"

void PeerRegistry::addKnown(const std::string& addr)
{
	Observer obs;
	{
		std::lock_guard<std::mutex> lock(_mutex);
		if (_known.insert(addr).second)
			obs = _on_discovered;
	}
	if (obs)
		obs(addr);
}

void PeerRegistry::addConnected(const std::string& addr, std::shared_ptr<NodeSession> session)
{
	Observer obs;
	{
		std::lock_guard<std::mutex> lock(_mutex);
		if (_known.insert(addr).second)
			obs = _on_discovered;
		_connected[addr] = std::move(session);
	}
	if (obs)
		obs(addr);
}

void PeerRegistry::removeConnected(const std::string& addr)
{
	std::lock_guard<std::mutex> lock(_mutex);
	_connected.erase(addr);
}

void PeerRegistry::setOnDiscovered(Observer obs)
{
	std::lock_guard<std::mutex> lock(_mutex);
	_on_discovered = std::move(obs);
}

bool PeerRegistry::isKnown(const std::string& addr) const
{
	std::lock_guard<std::mutex> lock(_mutex);
	return _known.count(addr) > 0;
}

bool PeerRegistry::isConnected(const std::string& addr) const
{
	std::lock_guard<std::mutex> lock(_mutex);
	return _connected.count(addr) > 0;
}

std::vector<std::string> PeerRegistry::known() const
{
	std::lock_guard<std::mutex> lock(_mutex);
	return { _known.begin(), _known.end() };
}

std::vector<std::string> PeerRegistry::connected() const
{
	std::lock_guard<std::mutex> lock(_mutex);
	std::vector<std::string> result;
	result.reserve(_connected.size());
	for (const auto& entry : _connected)
	{
		result.push_back(entry.first);
	}
	return result;
}

std::size_t PeerRegistry::knownCount() const
{
	std::lock_guard<std::mutex> lock(_mutex);
	return _known.size();
}

std::size_t PeerRegistry::connectedCount() const
{
	std::lock_guard<std::mutex> lock(_mutex);
	return _connected.size();
}
