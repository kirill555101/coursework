#!/usr/bin/bash
# Provides: guron
# Required-View-Cmd: $SERVER_NAME $cmd $key
### END INIT INFO


export SERVER_NAME=guron


get_pid() {
  PID_MASTER_PROCESS=$(head -n 1 pid_file.txt)
}


get_has_server_started() {
  HAS_SERVER_STARTED=$(ps aux | grep ./"$SERVER_NAME".out | wc -l)
}


start() {
  if [ ! -d cmake-build-debug ]; then
    echo "To start server you need to make 'sudo ./$SERVER_NAME.sh build'"
    exit 1
  fi

  get_has_server_started
  if [ "$HAS_SERVER_STARTED" \> 1 ]; then
    echo "Server has already started!"
    exit 1
  else
    if [ -f error.log ]; then
      rm error.log
    fi
    if [ -f access.log ]; then
      rm access.log
    fi
    touch access.log
    touch error.log
    echo "Starting $SERVER_NAME Server..."
    if [ -f "$SERVER_NAME".out ]; then
      rm "$SERVER_NAME".out
    fi
    cp ./cmake-build-debug/"$SERVER_NAME".out "$SERVER_NAME".out
    ./"$SERVER_NAME".out
    echo "Server started!"
    exit 0
  fi
}


stop_soft() {
  get_has_server_started
  if [ ! "$HAS_SERVER_STARTED" \> 1 ]; then
    echo "Server has not started yet!"
    exit 1
  else
    echo "Stopping soft $SERVER_NAME server..."
    get_pid
    kill -1 "$PID_MASTER_PROCESS"
    echo "Server stopped!"
    exit 0
	fi
}


stop_hard() {
  get_has_server_started
  if [ ! "$HAS_SERVER_STARTED" \> 1 ]; then
    echo "Server has not started yet!"
    exit 1
  else
    echo "Stopping hard $SERVER_NAME server..."
    get_pid
    kill -2 "$PID_MASTER_PROCESS"
    echo "Server stopped!"
    exit 0
  fi
}


reload_soft() {
  get_has_server_started
  if [ ! "$HAS_SERVER_STARTED" \> 1 ]; then
    echo "Server has not started yet!"
    exit 1
  else
    echo "Reloading soft $SERVER_NAME server..."
    get_pid
    kill -13 "$PID_MASTER_PROCESS"
    echo "Server reloaded!"
    exit 0
  fi
}


reload_hard() {
  get_has_server_started
  if [ ! "$HAS_SERVER_STARTED" \> 1 ]; then
    echo "Server has not started yet!"
    exit 1
  else
    echo "Reloading hard $SERVER_NAME server..."
    get_pid
    kill -14 "$PID_MASTER_PROCESS"
    echo "Server reloaded!"
    exit 0
  fi
}


status() {
  get_has_server_started
  if [ "$HAS_SERVER_STARTED" \> 1 ]; then
    echo "$SERVER_NAME server is running!"
  else
    echo "$SERVER_NAME server is down!"
  fi
}


create_config() {
  cp ."$SERVER_NAME".conf settings/"$SERVER_NAME".conf
}


build() {
  get_has_server_started
  if [ "$HAS_SERVER_STARTED" \> 1 ]; then
    echo "Can't build project while $SERVER_NAME server is running!"
    exit 1
  fi

  if [ -d cmake-build-debug ]; then
    cd cmake-build-debug
    make clean && make
    cd ..
  else
    mkdir cmake-build-debug
    cd cmake-build-debug
    cmake ..
    make clean && make
    cd ..
  fi
}


case $1 in
  start)
    start
  ;;

  stop)
    shift
    case $1 in
      soft)
        stop_soft
      ;;
      hard)
        stop_hard
      ;;
    esac
    echo "Usage : <hard|soft>";
  ;;

  reload)
    shift
    case $1 in
      soft)
        reload_soft
      ;;
      hard)
        reload_hard
      ;;
    esac
    echo "Usage : <hard|soft>";
  ;;

  status)
    status
  ;;

  build)
    build
  ;;

  create)
    shift
    if [ "$1" =  config ]; then
      create_config
    else
      echo "Usage : <config>";
    fi
  ;;

  *)
    get_has_server_started
    if [ "$HAS_SERVER_STARTED" \> 1 ]; then
      echo "Usage : <stop|reload|status>";
    else
      echo "Usage : <start|build|status|create>";
    fi
  ;;
esac
exit 0
