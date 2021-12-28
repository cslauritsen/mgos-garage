#!/bin/bash


test -f "$1" || {
 echo no such file $1
 exit 1
}

curl -v -F file=@$1 http://garage.planetlauritsen.com/update
