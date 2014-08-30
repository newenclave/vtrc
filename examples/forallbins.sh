#!/bin/bash

$1 examples/hello/client/hello_client
$1 examples/hello/server/hello_server

$1 examples/stress/client/stress_client
$1 examples/stress/server/stress_server


$1 examples/remote-fs/client/remote_fs_client
$1 examples/remote-fs/server/remote_fs_server


$1 examples/lukki-db/client/lukki_db_client
$1 examples/lukki-db/server/lukki_db_server


