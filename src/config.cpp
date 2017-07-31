/*
    NoirSocks-cli : command line interface program of NoirSocks
    Copyright (C) 2017  NoirSocks

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <yaml-cpp/yaml.h>
#include "config.h"

template<typename T>
T SafeAs(YAML::Node& node, const T& def_value)
{
    try
    {
        return node.as<T>();
    }
    catch (...) {}
    return def_value;
}

std::string SafeAsString(YAML::Node node, const std::string& def = "")
{
    return SafeAs<std::string>(node, def);
}

int SafeAsInt(YAML::Node node, int def = 0)
{
    return SafeAs<int>(node, def);
}

uint16_t SafeAsUShort(YAML::Node node, uint16_t def = 0)
{
    return SafeAs<uint16_t>(node, def);
}

bool LoadRouteEntry(YAML::Node route_conf, NoirSocks::Route& route, const std::set<std::string>& next_node_names)
{
    route.host_pattern = SafeAsString(route_conf["host_pattern"]);
    route.is_tunnel_exit = SafeAsInt(route_conf["exit_tunnel"]);
    if (route.is_tunnel_exit)
    {
        route.bind_addr = SafeAsString(route_conf["bind_addr"]);
        route.bind_port = SafeAsUShort(route_conf["bind_port_base"]);
        return true;
    }
    if (!route_conf["next_nodes"].IsSequence())
    {
        fprintf(stderr, "no next_nodes configured");
        return false;
    }
    for (size_t i = 0; i < route_conf["next_nodes"].size(); ++i)
    {
        std::string node_name = SafeAsString(route_conf["next_nodes"][i]);
        if (next_node_names.count(node_name) == 0)
        {
            fprintf(stderr, "invalid node name [%s] skip...", node_name.c_str());
            continue;
        }
        route.next_nodes.push_back(node_name);
    }
    if (route.next_nodes.empty())
    {
        fprintf(stderr, "no next_nodes configured");
        return false;
    }
    return true;
}

template<typename RouteHolder>
bool LoadRoute(YAML::Node& route_conf, RouteHolder& item, const std::set<std::string>& next_node_names, bool need_default = false)
{
    item.has_def_route = false;
    if (!route_conf["default"].IsMap())
    {
        if (need_default)
        {
            fprintf(stderr, "default route not configured");
            return false;
        }
    }
    else
    {
        item.has_def_route = true;
        if (!LoadRouteEntry(route_conf["default"], item.def_route, next_node_names))
        {
            if (need_default)
            {
                fprintf(stderr, "default route configure is invalid");
                return false;
            }
            else
            {
                item.has_def_route = false;
            }
        }
    }
    if (!route_conf["rules"].IsSequence())
    {
        if (item.has_def_route)
        {
            return true;
        }
        fprintf(stderr, "invalid route config : no route");
        return false;
    }
    for (size_t i = 0; i < route_conf["rules"].size(); ++i)
    {
        NoirSocks::Route route_item;
        if (LoadRouteEntry(route_conf["rules"][i], route_item, next_node_names))
        {
            if (route_item.host_pattern.size()) item.rules.push_back(route_item);
        }
    }
    if (item.rules.empty() && item.has_def_route == false)
    {
        fprintf(stderr, "invalid route config : no route");
        return false;
    }
    return true;
}

bool LoadConfig(const std::string& conf_file, NoirSocks::GlobalConfig& conf)
{
    YAML::Node conf_yaml = YAML::LoadFile(conf_file);
    if (!conf_yaml.IsMap())
    {
        fprintf(stderr, "conf %s is not a yaml map (or parse failed).", conf_file.c_str());
        return false;
    }

    //Load general configs
    if (!conf_yaml["general_config"])
    {
        fprintf(stderr, "general_config not found");
        return false;
    }
    YAML::Node general_config = conf_yaml["general_config"];
    conf.id = SafeAsString(general_config["id"]);
    if (conf.id.empty())
    {
        fprintf(stderr, "id not configured");
        return false;
    }
    std::string log_level = SafeAsString(general_config["log_level"]);
    if (log_level == "debug") conf.log_level = NoirSocks::LOG_LEVEL_DEBUG;
    else if (log_level == "info") conf.log_level = NoirSocks::LOG_LEVEL_INFO;
    else if (log_level == "error") conf.log_level = NoirSocks::LOG_LEVEL_ERROR;
    else if (log_level == "none") conf.log_level = NoirSocks::LOG_LEVEL_NONE;
    else {fprintf(stderr, "unrecognized log_level : %s", log_level.c_str()); return false;}
    conf.log_file = SafeAsString(general_config["log_file"]);
    conf.ban_stat_secs = SafeAsInt(general_config["defence_settings"]["stat_seconds"]);
    conf.ban_req_limit = SafeAsInt(general_config["defence_settings"]["bad_req_limit"]);
    conf.ban_list_file = SafeAsString(general_config["defence_settings"]["ban_list_file"]);

    //Load next nodes
    std::set<std::string> next_node_names;
    YAML::Node next_nodes = conf_yaml["next_nodes"];
    if (next_nodes.IsMap())
    {
        for (auto it = next_nodes.begin(); it != next_nodes.end(); ++it)
        {
            NoirSocks::NextNode node;
            node.name = SafeAsString(it->first);
            node.host = SafeAsString(it->second["host"]);
            int port = SafeAsInt(it->second["port"]);
            if (port < 1 || port >= 65535)
            {
                fprintf(stderr, "node [%s] config invalid : invalid port %d", node.name.c_str(), port);
                continue;
            }
            node.port = port;
            node.protocol = SafeAsString(it->second["protocol"]);

            node.psk = SafeAsString(it->second["psk"]);
            node.dynamic_iv_interval = SafeAsInt(it->second["dynamic_iv_interval"]);

            std::string err_msg;
            if (!NoirSocks::CheckProtocolConf(node, err_msg))
            {
                fprintf(stderr, "node [%s] config invalid : %s", node.name.c_str(), err_msg.c_str());
                continue;
            }
            conf.next_nodes[node.name] = node;
            next_node_names.insert(node.name);
        }
    }

    //Load global route
    YAML::Node route_conf = conf_yaml["route_conf"];
    if (!route_conf.IsMap())
    {
        fprintf(stderr, "global route not configured");
        return false;
    }
    if (!LoadRoute(route_conf, conf, next_node_names, true))
    {
        fprintf(stderr, "global route load failed");
        return false;
    }

    //Load local services
    YAML::Node services = conf_yaml["services"];
    if (!services.IsMap() || services.size() == 0)
    {
        fprintf(stderr, "local service not configured");
        return false;
    }
    for (auto it = services.begin(); it != services.end(); ++it)
    {
        NoirSocks::LocalService item;
        item.name = SafeAsString(it->first);
        item.host = SafeAsString(it->second["host"]);
        int port = SafeAsInt(it->second["port"]);
        if (port < 1 || port >= 65535)
        {
            fprintf(stderr, "service [%s] config invalid : invalid port %d", item.name.c_str(), port);
            continue;
        }
        item.port = port;
        item.protocol = SafeAsString(it->second["protocol"]);

        item.socks_udp_bind_port = SafeAsUShort(it->second["port"]);

        item.psk = SafeAsString(it->second["psk"]);
        item.dynamic_iv_interval = SafeAsInt(it->second["dynamic_iv_interval"]);

        std::string err_msg;
        if (!NoirSocks::CheckProtocolConf(item, err_msg))
        {
            fprintf(stderr, "service [%s] config invalid : %s", item.name.c_str(), err_msg.c_str());
            continue;
        }
        conf.local_services[item.name] = item;
    }

    return true;
}
