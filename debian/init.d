#! /bin/sh
#

PATH=/usr/local/sbin:/usr/local/bin:/sbin:/bin:/usr/sbin:/usr/bin
DAEMON=/usr/bin/gspl-start
NAME=GNUspool
DESC="GNUspool print spooler"
LABEL=GNUspool

test -x $DAEMON || exit 0

DODTIME=10                  # Time to wait for the server to die, in seconds
                            # If this value is set too low you might not
                            # let some servers to die gracefully and
                            # 'restart' will not work

# Include gnuspool defaults if available
if [ -f /etc/default/gnuspool ] ; then
	. /etc/default/gnuspool
fi

set -e

running()
{
    nproc=`pgrep spshed|wc -l`
    if [ "$nproc" -gt 0 ]
    then
	return 0
    else
	return 1
    fi
}

force_stop() {
# Forcefully kill the thing
    if running ; then
        pkill -15 spshed xtnetserv
        # Is it really dead?
        [ -n "$DODTIME" ] && sleep "$DODTIME"
        if running ; then
            pkill -9 spshed xtnetserv
            [ -n "$DODTIME" ] && sleep "$DODTIME"
            if running ; then
                echo "Cannot kill $LABEL!"
                exit 1
            fi
        fi
    fi
    return 0
}

case "$1" in
  start)
	echo -n "Starting $DESC: "

	# Delete previous memory-mapped stuff which might have got left behind

	rm -f $SPOOLDIR/spmm*

	# Start up with initial job and printer allocation from default
	
	$DAEMON $DAEMON_OPTS 2>/dev/null
        [ -n "$DODTIME" ] && sleep "$DODTIME"

        if running ; then
            echo "$NAME."
        else
            echo " ERROR."
        fi
	;;

  stop)
	echo -n "Stopping $DESC: "
	/usr/bin/gspl-stop -y 2>/dev/null
	echo "$NAME."
	;;

  force-stop)
	echo -n "Forcefully stopping $DESC: "
        force_stop
        if ! running ; then
            echo "$NAME."
        else
            echo " ERROR."
        fi
	;;

  force-reload)
	running && $0 restart || exit 0
	;;

  restart)
    echo -n "Restarting $DESC: "
	$0 stop
	[ -n "$DODTIME" ] && sleep $DODTIME
	$0 start
	echo "$NAME."
	;;

  status)
    echo -n "$LABEL is "
    if running ;  then
        echo "running"
    else
        echo " not running."
        exit 1
    fi
    ;;
  *)
	N=/etc/init.d/$NAME
	# echo "Usage: $N {start|stop|restart|reload|force-reload}" >&2
	echo "Usage: $N {start|stop|restart|force-reload|status|force-stop}" >&2
	exit 1
	;;
esac

exit 0
