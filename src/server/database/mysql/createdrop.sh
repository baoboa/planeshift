#!/bin/sh
# This scripts generate the sql commands to clear an already generate and
# perhaps messed up planeshift database

awk '/^CREATE TABLE/{print "DROP TABLE", $3 ";"}' *.sql > drop.sql
