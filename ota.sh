#!/bin/bash


test -f "$1" || {
 echo no such file $1
 exit 1
}

curl -v -F file=@$1 http://192.168.1.120/update
