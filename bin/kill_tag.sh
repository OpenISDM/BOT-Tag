#!/bin/bash

ps -ef | grep Tag | grep -v grep | awk '{print $2}' | xargs sudo kill -SIGINT
