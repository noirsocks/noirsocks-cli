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

#include <stdio.h>

#ifndef _WIN32
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#include <set>

#include "config.h"

#ifndef _WIN32
bool RunAsDaemon()
{
    pid_t child_pid = fork();
    if (child_pid < 0)
    {
        fprintf(stderr, "Error : fork() failed. errno = %d\n", errno);
        return false;
    }
    if (child_pid > 0)
    {
        exit(0);
    }

    setsid();
    if (chdir("/")){}
    umask(0);

    close(0);
    close(1);
    close(2);

    return true;
}
#endif

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s CONFIG_FILE\n", argv[0]);
        return -1;
    }

    //Load & check config
    NoirSocks::GlobalConfig conf;
    if (!LoadConfig(argv[1], conf))
    {
        fprintf(stderr, "Load config file %s failed", argv[1]);
        return -2;
    }

#ifndef _WIN32
    //Run as daemon
    if (!RunAsDaemon())
    {
        fprintf(stderr, "Cannot run as a daemon\n");
        return -3;
    }
#endif

    //Run NoirSocks
    return NoirSocks::GetServerInstance()->Run(conf);
}
