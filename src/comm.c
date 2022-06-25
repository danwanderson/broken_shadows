///////////////////////////////////////////////////////////////////////
////  Broken Shadows (c) 1995-1999 by Daniel Anderson
////  
////  Permission to use this code is given under the conditions set
////  forth in ../doc/shadows.license
////
////  For a complete list of credits, please see ../doc/shadows.credits
///////////////////////////////////////////////////////////////////////


/***************************************************************************
 *  Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,        *
 *  Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.   *
 *                                                                         *
 *  Merc Diku Mud improvments copyright (C) 1992, 1993 by Michael          *
 *  Chastain, Michael Quan, and Mitchell Tse.                              *
 *                                                                         *
 *  In order to use any part of this Merc Diku Mud, you must comply with   *
 *  both the original Diku license in 'license.doc' as well the Merc       *
 *  license in 'license.txt'.  In particular, you may not remove either of *
 *  these copyright notices.                                               *
 *                                                                         *
 *  Thanks to abaddon for proof-reading our comm.c and pointing out bugs.  *
 *  Any remaining bugs are, of course, our work, not his.  :)              *
 *                                                                         *
 *  Much time and thought has gone into this software and you are          *
 *  benefitting.  We hope that you share your changes too.  What goes      *
 *  around, comes around.                                                  *
 ***************************************************************************/

/*
 * This file contains all of the OS-dependent stuff:
 *   startup, signals, BSD sockets for tcp/ip, i/o, timing.
 *
 * The data flow for input is:
 *    Game_loop ---> Read_from_descriptor ---> Read
 *    Game_loop ---> Read_from_buffer
 *
 * The data flow for output is:
 *    Game_loop ---> Process_Output ---> Write_to_descriptor -> Write
 *
 * The OS-dependent functions are Read_from_descriptor and Write_to_descriptor.
 * -- Furey  26 Jan 1993
 */

#include <sys/types.h>
#include <sys/time.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <form.h>
#include "merc.h"
#include <math.h>
#include "recycle.h"
#include "interp.h"
#include <unistd.h>
#include <pwd.h>

#define CH(descriptor)  ((descriptor)->original ? (descriptor)->original : (descriptor)->character)
/* This file holds the copyover data */
#define COPYOVER_FILE "copyover.data"

/* This is the executable file */
#define EXE_FILE          "../bin/shadows"

/*
 * execl from unistd.h. The guy who wrote copyover forgot to put a
 * prototype for this function in and it made a warning. I hate warnings
 * almost as much as I hate errors. -Rahl
 */
extern int execl __P ((__const char *__path, __const char *__arg,...));


#include <signal.h>

/*
 * Socket and TCP/IP stuff.
 */
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/telnet.h>
const   char    go_ahead_str    [] = { IAC, GA, '\0' };



/*
 * OS-dependent declarations.
 */

#if     defined(linux)

int     close           ( int fd );
//int     gettimeofday    ( struct timeval *tp, struct timezone *tzp );
//int     read            ( int fd, char *buf, int nbyte );
int     select          ( int width, fd_set *readfds, fd_set *writefds,
                            fd_set *exceptfds, struct timeval *timeout );
int     socket          ( int domain, int type, int protocol );
//int     write           ( int fd, char *buf, int nbyte );
#endif

/* This includes Solaris Sys V as well */
#if defined(sun)
int     accept          ( int s, struct sockaddr *addr, int *addrlen );
int     bind            ( int s, struct sockaddr *name, int namelen );
void    bzero           ( char *b, int length );
int     close           ( int fd );
int     getpeername     ( int s, struct sockaddr *name, int *namelen );
int     getsockname     ( int s, struct sockaddr *name, int *namelen );
int     gettimeofday    ( struct timeval *tp, struct timezone *tzp );
int     listen          ( int s, int backlog );
int     read            ( int fd, char *buf, int nbyte );
int     select          ( int width, fd_set *readfds, fd_set *writefds,
                            fd_set *exceptfds, struct timeval *timeout );
#if defined(SYSV)
int setsockopt          ( int s, int level, int optname,
                            const char *optval, int optlen );
#else
int     setsockopt      ( int s, int level, int optname, void *optval,
                            int optlen );
#endif
int     socket          ( int domain, int type, int protocol );
int     write           ( int fd, char *buf, int nbyte );
#endif

/*
 * Global variables.
 */
DESCRIPTOR_DATA *   descriptor_free;    /* Free list for descriptors    */
DESCRIPTOR_DATA *   descriptor_list;    /* All open descriptors         */
DESCRIPTOR_DATA *   d_next;             /* Next descriptor in loop      */
FILE *              fpReserve;          /* Reserved file handle         */
bool                merc_down;          /* Shutdown                     */
bool                wizlock;            /* Game is wizlocked            */
bool                newlock;            /* Game is newlocked            */
bool                chaos;              /* Game in CHAOS!               */
bool				DEBUG;				/* game is in debug mode */
char                str_boot_time[MAX_INPUT_LENGTH];
time_t              current_time;       /* time of this pulse */

/*
 * OS-dependent local functions.
 */
void    game_loop               ( int control );
int     init_socket             ( int port ); 
void    new_descriptor          ( int control );
bool    read_from_descriptor    ( DESCRIPTOR_DATA *d, bool color );
bool    write_to_descriptor     ( int desc, char *txt, int length, bool color);
void    copyover_recover        ();

/*
 * Other local functions (OS-independent).
 */
int     main                    ( int argc, char **argv );
void    nanny                   ( DESCRIPTOR_DATA *d, char *argument );
bool    process_output          ( DESCRIPTOR_DATA *d, bool fPrompt );
void    read_from_buffer        ( DESCRIPTOR_DATA *d, bool color );
void    stop_idling             ( CHAR_DATA *ch );
char    *do_color               (char *plaintext, bool color);
char    *doparseprompt          (CHAR_DATA *ch);


int port, control;

int main( int argc, char **argv )
{
    struct timeval now_time;
    bool fCopyOver = FALSE;

    /*
     * Init time.
     */
    gettimeofday( &now_time, NULL );
    current_time        = (time_t) now_time.tv_sec;
    strcpy( str_boot_time, ctime( &current_time ) );

    /*
     * Reserve one channel for our use.
     */
    if ( ( fpReserve = fopen( NULL_FILE, "r" ) ) == NULL )
    {
        perror( NULL_FILE );
        exit( 1 );
    }

    /*
     * Get the port number.
     */
    port = 4000;
    if ( argc > 1 )
    {
        if ( !is_number( argv[1] ) )
        {
            fprintf( stderr, "Usage: %s [port #]\n", argv[0] );
            exit( 1 );
        }
        else if ( ( port = atoi( argv[1] ) ) <= 1024 )
        {
            fprintf( stderr, "Port number must be above 1024.\n" );
            exit( 1 );
        }

        /* Are we recovering from a copyover? */
        if (argv[2] && argv[2][0])
        {
                fCopyOver = TRUE;
                control = atoi(argv[3]);
        }
        else
                fCopyOver = FALSE;

    }


    /*
     * Run the game.
     */
    if (!fCopyOver)
        control = init_socket( port );

    boot_db( );
    sprintf( log_buf, "Broken Shadows is ready to rock on port %d.", port );
    log_string( log_buf );

    if (fCopyOver)
        copyover_recover();


    game_loop( control );
    close (control);

    /*
     * That's all, folks.
     */
    log_string( "Normal termination of game." );
    /* added by Rahl */
    exit( 0 );
    return 0;
}

int init_socket( int port )
{
    static struct sockaddr_in sa_zero;
    struct sockaddr_in sa;
    int x = 1;
    int fd;

    if ( ( fd = socket( AF_INET, SOCK_STREAM, 0 ) ) < 0 )
    {
        perror( "Init_socket: socket" );
        exit( 1 );
    }

    if ( setsockopt( fd, SOL_SOCKET, SO_REUSEADDR,
    (char *) &x, sizeof(x) ) < 0 )
    {
        perror( "Init_socket: SO_REUSEADDR" );
        close(fd);
        exit( 1 );
    }

#if defined(SO_DONTLINGER) && !defined(SYSV)
    {
        struct  linger  ld;

        ld.l_onoff  = 1;
        ld.l_linger = 1000;

		fprintf( stderr, "Init_socket: SO_DONTLINGER defined" );

        if ( setsockopt( fd, SOL_SOCKET, SO_DONTLINGER,
        (char *) &ld, sizeof(ld) ) < 0 )
        {
            perror( "Init_socket: SO_DONTLINGER" );
            close(fd);
            exit( 1 );
        }
    }
#endif

    sa              = sa_zero;
    sa.sin_family   = AF_INET;
    sa.sin_port     = htons( port );

    if ( bind( fd, (struct sockaddr *) &sa, sizeof(sa) ) < 0 )
    {
        perror("Init socket: bind" );
        close(fd);
        exit(1);
    }


    if ( listen( fd, 3 ) < 0 )
    {
        perror("Init socket: listen");
        close(fd);
        exit(1);
    }

    return fd;
}

void game_loop( int control )
{
    static struct timeval null_time;
    struct timeval last_time;
    bool color;

    signal( SIGPIPE, SIG_IGN );
    gettimeofday( &last_time, NULL );
    current_time = (time_t) last_time.tv_sec;

	if ( geteuid() == 0 ) {
		setuid( getpwnam( "dwa" )->pw_uid );
	}

    /* Main loop */
    while ( !merc_down )
    {
        fd_set in_set;
        fd_set out_set;
        fd_set exc_set;
        DESCRIPTOR_DATA *d;
        int maxdesc;

        /*
         * Poll all active descriptors.
         */
        FD_ZERO( &in_set  );
        FD_ZERO( &out_set );
        FD_ZERO( &exc_set );
        FD_SET( control, &in_set );
        maxdesc = control;
        for ( d = descriptor_list; d; d = d->next )
        {
            maxdesc = UMAX( maxdesc, d->descriptor );
            FD_SET( d->descriptor, &in_set  );
            FD_SET( d->descriptor, &out_set );
            FD_SET( d->descriptor, &exc_set );
        }

        if ( select( maxdesc+1, &in_set, &out_set, &exc_set, &null_time ) < 0 )
        {
            perror( "Game_loop: select: poll" );
            exit( 1 );
        }

        /*
         * New connection?
         */
        if ( FD_ISSET( control, &in_set ) )
            new_descriptor( control );

        /*
         * Kick out the freaky folks.
         */
        for ( d = descriptor_list; d != NULL; d = d_next )
        {
            d_next = d->next;
            if ( FD_ISSET( d->descriptor, &exc_set ) )
            {
                FD_CLR( d->descriptor, &in_set  );
                FD_CLR( d->descriptor, &out_set );
                if ( d->character && d->character->level > 1)
                    save_char_obj( d->character );
                d->outtop       = 0;
                close_socket( d );
            }
        }

        /*
         * Process input.
         */
        for ( d = descriptor_list; d != NULL; d = d_next )
        {
            d_next      = d->next;
            d->fcommand = FALSE;

            if ( FD_ISSET( d->descriptor, &in_set ) )
            {
                if ( d->character != NULL ) {
                    d->character->timer = 0;
                    if ( IS_SET(d->character->act, PLR_COLOR))
                        color=TRUE;
                    else
                        color=FALSE;
                } else color=FALSE;
                if ( !read_from_descriptor( d, color ) )
                {
                    FD_CLR( d->descriptor, &out_set );
                    if ( d->character != NULL && d->character->level > 1)
                        save_char_obj( d->character );
                    d->outtop   = 0;
                    close_socket( d );
                    continue;
                }
            }

            if ( d->character != NULL && d->character->wait > 0 )
            {
                --d->character->wait;
                continue;
            }

            read_from_buffer( d, FALSE );
            if ( d->incomm[0] != '\0' )
            {
                d->fcommand     = TRUE;
                stop_idling( d->character );

        /* OLC */
        if ( d->showstr_point )
            show_string( d, d->incomm );
        else
        if ( d->pString )
            string_add( d->character, d->incomm );
        else
            switch ( d->connected )
            {
                case CON_PLAYING:
                    if ( !run_olc_editor( d ) )
                        substitute_alias( d, d->incomm );
                    break;
                default:
                    nanny( d, d->incomm );
                    break;
            }

                d->incomm[0]    = '\0';
            }
        }

        /*
         * Autonomous game motion.
         */
        update_handler( );

        /*
         * Output.
         */
        for ( d = descriptor_list; d != NULL; d = d_next )
        {
            d_next = d->next;

            if ( ( d->fcommand || d->outtop > 0 )
            &&   FD_ISSET(d->descriptor, &out_set) )
            {
                if ( !process_output( d, TRUE ) )
                {
                    if ( d->character != NULL && d->character->level > 1)
                        save_char_obj( d->character );
                    d->outtop   = 0;
                    close_socket( d );
                }
            }
        }



        /*
         * Synchronize to a clock.
         * Sleep( last_time + 1/PULSE_PER_SECOND - now ).
         * Careful here of signed versus unsigned arithmetic.
         */
        {
            struct timeval now_time;
            long secDelta;
            long usecDelta;

            gettimeofday( &now_time, NULL );
            usecDelta   = ((int) last_time.tv_usec) - ((int) now_time.tv_usec)
                        + 1000000 / PULSE_PER_SECOND;
            secDelta    = ((int) last_time.tv_sec ) - ((int) now_time.tv_sec );
            while ( usecDelta < 0 )
            {
                usecDelta += 1000000;
                secDelta  -= 1;
            }

            while ( usecDelta >= 1000000 )
            {
                usecDelta -= 1000000;
                secDelta  += 1;
            }

            if ( secDelta > 0 || ( secDelta == 0 && usecDelta > 0 ) )
            {
                struct timeval stall_time;

                stall_time.tv_usec = usecDelta;
                stall_time.tv_sec  = secDelta;
                if ( select( 0, NULL, NULL, NULL, &stall_time ) < 0 )
                {
                    perror( "Game_loop: select: stall" );
                    exit( 1 );
                }
            }
        }

        gettimeofday( &last_time, NULL );
        current_time = (time_t) last_time.tv_sec;
    }
    return;
}



void init_descriptor( DESCRIPTOR_DATA *dnew, int desc)
{
    static DESCRIPTOR_DATA d_zero;

    *dnew               = d_zero;
    dnew->descriptor    = desc;
    dnew->connected     = CON_ANSI;
    dnew->showstr_head  = NULL;
    dnew->showstr_point = NULL;
    dnew->outsize       = 2000;
    dnew->pEdit         = NULL;                 /* OLC */
    dnew->pString       = NULL;                 /* OLC */
    dnew->editor        = 0;                    /* OLC */
    dnew->outbuf        = malloc( dnew->outsize );
}


void new_descriptor( int control )
{
    static DESCRIPTOR_DATA d_zero;
    char buf[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *dnew;
    struct sockaddr_in sock;
    struct hostent *from;
    int desc;
    int size;
    bool color;

    buf[0] = '\0';

    size = sizeof(sock);
    getsockname( control, (struct sockaddr *) &sock, &size );
    if ( ( desc = accept( control, (struct sockaddr *) &sock, &size) ) <0 )
    {
        perror( "New_descriptor: accept" );
        return;
    }

#if !defined(FNDELAY)
#define FNDELAY O_NDELAY
#endif

    if ( fcntl( desc, F_SETFL, FNDELAY ) == -1 )
    {
        perror( "New_descriptor: fcntl: FNDELAY" );
        return;
    }

    /*
     * Cons a new descriptor.
     */
    if ( descriptor_free == NULL )
    {
        dnew            = alloc_perm( sizeof(*dnew) );
    }
    else
    {
        dnew            = descriptor_free;
        descriptor_free = descriptor_free->next;
    }

    *dnew               = d_zero;
    dnew->descriptor    = desc;
    dnew->connected     = CON_ANSI;
    dnew->showstr_head  = NULL;
    dnew->showstr_point = NULL;
    dnew->outsize       = 2000;
    dnew->pEdit         = NULL;                 /* OLC */
    dnew->pString       = NULL;                 /* OLC */
    dnew->editor        = 0;                    /* OLC */
    dnew->outbuf        = malloc( dnew->outsize );


    size = sizeof(sock);
    if ( getpeername( desc, (struct sockaddr *) &sock, &size ) < 0 )
    {
        perror( "New_descriptor: getpeername" );
        dnew->host = str_dup( "(unknown)" );
    }
    else
    {
        /*
         * Would be nice to use inet_ntoa here but it takes a struct arg,
         * which ain't very compatible between gcc and system libraries.
         */
        int addr;

        addr = ntohl( sock.sin_addr.s_addr );
        sprintf( buf, "%d.%d.%d.%d",
            ( addr >> 24 ) & 0xFF, ( addr >> 16 ) & 0xFF,
            ( addr >>  8 ) & 0xFF, ( addr       ) & 0xFF
            );
        sprintf( log_buf, "Sock.sinaddr:  %s", buf );
        log_string( log_buf );
        from = gethostbyaddr( (char *) &sock.sin_addr,
            sizeof(sock.sin_addr), AF_INET );
        if (from && (!str_cmp(from->h_name,"ursula.uoregon.edu")
                 ||  !str_cmp(from->h_name,"monet.ucdavis.edu")))
            dnew->host = str_dup("white.nextwork.rose-hulman.edu");
        else
            dnew->host = str_dup( from ? from->h_name : buf );
    }

    /*
     * Swiftest: I added the following to ban sites.  I don't
     * endorse banning of sites, but Copper has few descriptors now
     * and some people from certain sites keep abusing access by
     * using automated 'autodialers' and leaving connections hanging.
     *
     * Furey: added suffix check by request of Nickel of HiddenWorlds.
     */
    if ( check_ban(dnew->host,BAN_ALL))
    {
        color = TRUE;
        write_to_descriptor( desc,
            "Your site has been banned from Broken Shadows.\n\r", 0, color );
        close( desc );
        free_string(dnew->host);
        free(dnew->outbuf);
        dnew->next      = descriptor_free;
        descriptor_free = dnew;
        return;
    }


    /*
     * Init descriptor data.
     */
    dnew->next                  = descriptor_list;
    descriptor_list             = dnew;

    /*
     * Send the greeting.
     */
/*
    {
        extern char * help_greeting;
        if ( help_greeting[0] == '.' )
            write_to_buffer( dnew, help_greeting+1, 0 );
        else
            write_to_buffer( dnew, help_greeting  , 0 );
    }
*/

	write_to_buffer( dnew, "\n\rDo you want ANSI color? [Y/n] ", 0 );

    return;
}



void close_socket( DESCRIPTOR_DATA *dclose )
{
    CHAR_DATA *ch;

    if ( dclose->outtop > 0 )
        process_output( dclose, FALSE );

    if ( dclose->snoop_by != NULL )
    {
        write_to_buffer( dclose->snoop_by,
            "Your victim has left the game.\n\r", 0 );
    }

    {
        DESCRIPTOR_DATA *d;

        for ( d = descriptor_list; d != NULL; d = d->next )
        {
            if ( d->snoop_by == dclose )
                d->snoop_by = NULL;
        }
    }

    if ( ( ch = dclose->character ) != NULL )
    {
        sprintf( log_buf, "Closing link to %s.", ch->name );
        log_string( log_buf );
        /*
         * if ch is writing a note or playing, just lose link otherwise
         * clear char
         */
        if ( ( dclose->connected == CON_PLAYING ) ||
             ( ( dclose->connected >= CON_NOTE_TO ) &&
                ( dclose->connected <= CON_NOTE_FINISH ) ) )
        {
            act( "$n has lost $s link.", ch, NULL, NULL, TO_ROOM );
            /* added by Rahl */
            wiznet( "Net death has claimed $N.", ch, NULL, WIZ_LINKS, 0, 0);
            ch->desc = NULL;
        }
        else
        {
            free_char( dclose->character );
        }
    }

    if ( d_next == dclose )
        d_next = d_next->next;

    if ( dclose == descriptor_list )
    {
        descriptor_list = descriptor_list->next;
    }
    else
    {
        DESCRIPTOR_DATA *d;

        for ( d = descriptor_list; d && d->next != dclose; d = d->next )
            ;
        if ( d != NULL )
            d->next = dclose->next;
        else
            bug( "Close_socket: dclose not found.", 0 );
    }

    close( dclose->descriptor );
    free_string( dclose->host );
    /* RT socket leak fix -- I hope */
    free(dclose->outbuf );
/*    free_string(dclose->showstr_head); */
    dclose->next        = descriptor_free;
    descriptor_free     = dclose;
    return;
}



bool read_from_descriptor( DESCRIPTOR_DATA *d, bool color )
{
    int iStart;

    /* Hold horses if pending command already. */
    if ( d->incomm[0] != '\0' )
        return TRUE;

    /* Check for overflow. */
    iStart = strlen(d->inbuf);
    if ( iStart >= sizeof(d->inbuf) - 10 )
    {
        sprintf( log_buf, "%s input overflow!", d->host );
        log_string( log_buf );
        write_to_descriptor( d->descriptor,
            "\n\r*** PUT A LID ON IT!!! ***\n\r", 0 , color);
        return FALSE;
    }

    /* Snarf input. */

    for ( ; ; )
    {
        int nRead;

        nRead = read( d->descriptor, d->inbuf + iStart,
            sizeof(d->inbuf) - 10 - iStart );
        if ( nRead > 0 )
        {
            iStart += nRead;
            if ( d->inbuf[iStart-1] == '\n' || d->inbuf[iStart-1] == '\r' )
                break;
        }
        else if ( nRead == 0 )
        {
            log_string( "EOF encountered on read." );
            return FALSE;
        }
        else if ( errno == EWOULDBLOCK )
            break;
        else
        {
            perror( "Read_from_descriptor" );
            return FALSE;
        }
    }

    d->inbuf[iStart] = '\0';
    return TRUE;
}



/*
 * Transfer one line from input buffer to input line.
 */
void read_from_buffer( DESCRIPTOR_DATA *d, bool color )
{
    int i, j, k;

    /*
     * Hold horses if pending command already.
     */
    if ( d->incomm[0] != '\0' )
        return;

    /*
     * Look for at least one new line.
     */
    for ( i = 0; d->inbuf[i] != '\n' && d->inbuf[i] != '\r'; i++ )
    {
        if ( d->inbuf[i] == '\0' )
            return;
    }

    /*
     * Canonical input processing.
     */
    for ( i = 0, k = 0; d->inbuf[i] != '\n' && d->inbuf[i] != '\r'; i++ )
    {
        if ( k >= MAX_INPUT_LENGTH - 2 )
        {
            write_to_descriptor( d->descriptor, "Line too long.\n\r", 0 , color);

            /* skip the rest of the line */
            for ( ; d->inbuf[i] != '\0'; i++ )
            {
                if ( d->inbuf[i] == '\n' || d->inbuf[i] == '\r' )
                    break;
            }
            d->inbuf[i]   = '\n';
            d->inbuf[i+1] = '\0';
            break;
        }

        if ( d->inbuf[i] == '\b' && k > 0 )
            --k;
        else if ( isascii(d->inbuf[i]) && isprint(d->inbuf[i]) )
            d->incomm[k++] = d->inbuf[i];
    }

    /*
     * Finish off the line.
     */
    if ( k == 0 )
        d->incomm[k++] = ' ';
    d->incomm[k] = '\0';

    /*
     * Deal with bozos with #repeat 1000 ...
     */


    if ( k > 1 || d->incomm[0] == '!' )
    {
        if ( d->incomm[0] != '!' && strcmp( d->incomm, d->inlast ) )
        {
            d->repeat = 0;
        }
        else
        {
            if ( ++d->repeat >= 25 )
            {
                /* changed from d->host by Rahl */
                sprintf( log_buf, "%s input spamming!", d->character->name );
                log_string( log_buf );
                /* added by Rahl */
                wiznet( log_buf, d->character, NULL, WIZ_SPAM, 0,
                        get_trust(d->character) );
                if ( d->incomm[0] == '!' )
                {
                    wiznet(d->inlast, d->character, NULL, WIZ_SPAM, 0,
                        get_trust(d->character) );
                    log_string( d->inlast );
                }
                else
                {
                    wiznet(d->incomm, d->character, NULL, WIZ_SPAM, 0,
                        get_trust(d->character) );
                    log_string( d->incomm );
                }
                d->repeat = 0;
/*
                write_to_descriptor( d->descriptor,
                    "\n\r*** PUT A LID ON IT!!! ***\n\r", 0 , color);
                strcpy( d->incomm, "quit" );
 */
            }
        }
    }


    /*
     * Do '!' substitution.
     */
    if ( d->incomm[0] == '!' )
        strcpy( d->incomm, d->inlast );
    else
        strcpy( d->inlast, d->incomm );

    /*
     * Shift the input buffer.
     */
    while ( d->inbuf[i] == '\n' || d->inbuf[i] == '\r' )
        i++;
    for ( j = 0; ( d->inbuf[j] = d->inbuf[i+j] ) != '\0'; j++ )
        ;
    return;
}



/*
 * Low level output function.
 */
bool process_output( DESCRIPTOR_DATA *d, bool fPrompt )
{
    extern bool merc_down;
    bool color=TRUE;

    /*
     * Bust a prompt.
     */
    if ( (d->character != NULL) && IS_SET(d->character->act,PLR_COLOR))
        color=TRUE;
      else
        color=FALSE;
    if ( !merc_down ) 
        if ( d->showstr_point )
            write_to_buffer( d, "`W[Hit Return to continue]\n\r`w", 0 );
        else if ( fPrompt && d->pString && d->connected == CON_PLAYING )
            write_to_buffer( d, "> ", 2 );
        else if ( fPrompt && d->connected == CON_PLAYING )
    {
        CHAR_DATA *ch;

        ch = d->character;
        if (IS_SET(ch->act,PLR_COLOR))
            color=TRUE;
          else
            color=FALSE;


        ch = d->original ? d->original : d->character;
        if (!IS_SET(ch->comm, COMM_COMPACT) )
            write_to_buffer( d, "\n\r", 2 );

        if ( IS_SET(ch->comm, COMM_PROMPT) )
        {
            BUFFER *buf = buffer_new( 200 );

            ch = d->character;
            if (!IS_NPC(ch))
            {
                bprintf( buf, "%s`w", doparseprompt(ch));
            }
            else bprintf( buf, "<H%d/%d M%d/%d V%d/%d> ",
                ch->hit, ch->max_hit,
                ch->mana, ch->max_mana, ch->move, ch->max_move );
            write_to_buffer( d, buf->data, 0 );
            buffer_free( buf );
            if ( ch->prefix[0] != '\0' )
            {
                write_to_buffer( d, ch->prefix, 0 );
                write_to_buffer( d, " ", 0 );
            }
        }

        if (IS_SET(ch->comm,COMM_TELNET_GA))
            write_to_buffer(d,go_ahead_str,0);
    }

    /*
     * Short-circuit if nothing to write.
     */
    if ( d->outtop == 0 )
        return TRUE;

    /*
     * Snoop-o-rama.
     */
    if ( d->snoop_by != NULL )
    {
        if (d->character != NULL)
            write_to_buffer( d->snoop_by, d->character->name,0);
        write_to_buffer( d->snoop_by, "> ", 2 );
        write_to_buffer( d->snoop_by, d->outbuf, d->outtop );
    }

    /*
     * OS-dependent output.
     */
    if ( !write_to_descriptor( d->descriptor, d->outbuf, d->outtop , color) )
    {
        d->outtop = 0;
        return FALSE;
    }
    else
    {
        d->outtop = 0;
        return TRUE;
    }
	
}



/*
 * Append onto an output buffer.
 */
 void write_to_buffer( DESCRIPTOR_DATA *d, const char *txt, int length )
{

    /*
     * Find length in case caller didn't.
     */
    if ( length <= 0 )
        length = strlen(txt);

    /*
     * Initial \n\r if needed.
     */
    if ( d->outtop == 0 && !d->fcommand )
    {
        d->outbuf[0]    = '\n';
        d->outbuf[1]    = '\r';
        d->outtop       = 2;
    }

    /*
     * Expand the buffer as needed.
     */
    while ( d->outtop + length >= d->outsize )
    {
        char *outbuf;

        if (d->outsize > 32000)
        {
            bug("Buffer overflow. Closing.\n\r",0);
            close_socket(d);
            return;
        }
        outbuf      = malloc( 2 * d->outsize );
        strncpy( outbuf, d->outbuf, d->outtop );
        free( d->outbuf );
        d->outbuf   = outbuf;
        d->outsize *= 2;
    }

    /*
     * Copy.
     */

    strcpy( d->outbuf + d->outtop, txt );
    d->outtop += length;
    return;
}



/*
 * Lowest level output function.
 * Write a block of text to the file descriptor.
 * If this gives errors on very long blocks (like 'ofind all'),
 *   try lowering the max block size.
 */
bool write_to_descriptor( int desc, char *txt, int length , bool color)
{
    int iStart;
    int nWrite;
    int nBlock;
/*!!!!   Lines added by ZAK to prepare for color stuff*/
    char blah[MAX_STRING_LENGTH*2];

    strncpy(blah, do_color(txt, color), sizeof(blah)-1);
    strcat (blah, "\0");

  /*  if ( length <= 0 )  */
        length = strlen(blah);
    for ( iStart = 0; iStart < length; iStart += nWrite )
    {
        nBlock = UMIN( length - iStart, sizeof(blah) );
        if ( ( nWrite = write( desc, blah + iStart, nBlock ) ) < 0 )
            { perror( "Write_to_descriptor" ); return FALSE; }
    }
/*
        if ( ( nWrite = write( desc, txt + iStart, nBlock ) ) < 0 )
            { perror( "Write_to_descriptor" ); return FALSE; }
*/

    return TRUE;
}



/*
 * Deal with sockets that haven't logged in yet.
 */
void nanny( DESCRIPTOR_DATA *d, char *argument ) {
    CHAR_DATA *ch;

    /* Delete leading spaces unless char is writing a note */
    if ( d->connected != CON_NOTE_TEXT )
    {
        while ( isspace(*argument) )
            argument++;
    }

    ch = d->character;

    switch ( d->connected ) {

    	default:
        	bug( "Nanny: bad d->connected %d.", d->connected );
        	close_socket( d );
        	return;

	    case CON_GET_NAME:
			handle_con_get_name( d, argument );
        	break;

	    case CON_GET_OLD_PASSWORD:
			handle_con_get_old_password( d, argument );
        	break;

		/* RT code for breaking link */
    	case CON_BREAK_CONNECT:
			handle_con_break_connect( d, argument );
	        break;

    	case CON_CONFIRM_NEW_NAME:
			handle_con_confirm_new_name( d, argument );
	        break;

   		case CON_GET_NEW_PASSWORD:
			handle_con_get_new_password( d, argument );
        	break;

    	case CON_CONFIRM_NEW_PASSWORD:
			handle_con_confirm_new_password( d, argument );
        	break;

	    case CON_GET_NEW_RACE:
			handle_con_get_new_race( d, argument );	
        	break;

    	case CON_GET_NEW_SEX:
			handle_con_get_new_sex( d, argument );
        	break;

	    case CON_GET_STATS:
			handle_con_get_stats( d, argument );
    	    break;


    	case CON_GET_NEW_CLASS:
			handle_con_get_new_class( d, argument );
	        break;

	    case CON_GET_ALIGNMENT:
			handle_con_get_alignment( d, argument );
    	    break;

	    case CON_DEFAULT_CHOICE:
			handle_con_default_choice( d, argument );
    	    break;

	    case CON_PICK_WEAPON:
			handle_con_pick_weapon( d, argument );
    	    break;

    	case CON_GEN_GROUPS:
			handle_con_gen_groups( d, argument );
        	break;

    	case CON_BEGIN_REMORT:
			handle_con_begin_remort( d, argument );
        	break;

	    case CON_READ_IMOTD:
			handle_con_read_imotd( d, argument );
    	    break;

		case CON_ANSI:
			handle_con_ansi( d, argument );
			break;

    	case CON_READ_MOTD:
			handle_con_read_motd( d, argument );
        	break;

        /* states for new note system, (c)1995-96 erwin@pip.dknet.dk */
        /* ch MUST be PC here; have nwrite check for PC status! */

        case CON_NOTE_TO:
            handle_con_note_to (d, argument);
            break;

        case CON_NOTE_SUBJECT:
            handle_con_note_subject (d, argument);
            break; /* subject */

        case CON_NOTE_EXPIRE:
            handle_con_note_expire (d, argument);
            break;

        case CON_NOTE_TEXT:
            handle_con_note_text (d, argument);
            break;

        case CON_NOTE_FINISH:
            handle_con_note_finish (d, argument);
            break;
    } // switch
    return;
}



/*
 * Parse a name for acceptability.
 */
bool check_parse_name( char *name )
{
    CLAN_DATA *pClan;

    /*
     * Reserved words.
     */
    if ( is_exact_name( name, "all auto immortal self someone something the you fuck shit" )
       || is_name(name, "fuck") || is_name(name, "shit") || is_name(name, "imm") )
        return FALSE;

    if ( is_name( name, "broken" ) || is_name( name, "shadows" )
        || is_exact_name( name, "in help" ) || is_name( name, "poop" ) )
        return FALSE;

    for ( pClan = clan_first; pClan != NULL; pClan = pClan->next )
        if ( is_name( name, pClan->name ) )
            return FALSE;

    if (str_cmp(capitalize(name),"Alander") && (!str_prefix("aland",name)
    || !str_suffix("alander",name)))
        return FALSE;

    /*
     * Length restrictions.
     */

    if ( strlen(name) <  2 )
        return FALSE;


    if ( strlen(name) > 12 )
        return FALSE;

	/*
 	 * Make sure someone isn't creating a char with the same name
	 */
	{
		DESCRIPTOR_DATA *d;

		for ( d = descriptor_list; d != NULL; d = d->next ) {
			if ( d->character && ( d->connected > CON_PLAYING ) ) {
				if ( !str_cmp( d->original ? d->original->name :
					d->character ? d->character->name : "None", name ) ) {
					return FALSE;
				}
			}
		}
	}

    /*
     * Alphanumerics only.
     * Lock out IllIll twits.
     */
    {
        char *pc;
        bool fIll;

        fIll = TRUE;
        for ( pc = name; *pc != '\0'; pc++ )
        {
            if ( !isalpha(*pc) )
                return FALSE;
            if ( LOWER(*pc) != 'i' && LOWER(*pc) != 'l' )
                fIll = FALSE;
        }

        if ( fIll )
            return FALSE;
    }

    /*
     * Prevent players from naming themselves after mobs.
     */
    {
        extern MOB_INDEX_DATA *mob_index_hash[MAX_KEY_HASH];
        MOB_INDEX_DATA *pMobIndex;
        int iHash;

        for ( iHash = 0; iHash < MAX_KEY_HASH; iHash++ )
        {
            for ( pMobIndex  = mob_index_hash[iHash];
                  pMobIndex != NULL;
                  pMobIndex  = pMobIndex->next )
            {
                if ( is_exact_name( name, pMobIndex->player_name ) )
                    return FALSE;
            }
        }
    }

    return TRUE;
}



/*
 * Look for link-dead player to reconnect.
 */
bool check_reconnect( DESCRIPTOR_DATA *d, char *name, bool fConn )
{
    CHAR_DATA *ch;
    char buf[100];

    buf[0] = '\0';

    for ( ch = char_list; ch != NULL; ch = ch->next )
    {
        if ( !IS_NPC(ch)
        &&   (!fConn || ch->desc == NULL)
        &&   !str_cmp( d->character->name, ch->name ) )
        {
            if ( fConn == FALSE )
            {
                free_string( d->character->pcdata->pwd );
                d->character->pcdata->pwd = str_dup( ch->pcdata->pwd );
            }
            else
            {
                free_char( d->character );
                d->character = ch;
                ch->desc         = d;
                ch->timer        = 0;
                send_to_char( "Reconnecting.\n\r", ch );
                if ( ch->pcdata->message_ctr )
                {
                    sprintf( buf, "You have %d messages waiting.\n\r",
                        ch->pcdata->message_ctr );
                    send_to_char( buf, ch );
                }
                act( "$n has reconnected.", ch, NULL, NULL, TO_ROOM );
                sprintf( log_buf, "%s@%s reconnected.", ch->name, d->host );
                /* added by Rahl */
                wiznet( "$N's link is restored.", ch, NULL, WIZ_LINKS,0, 0 );
                log_string( log_buf );
                d->connected = CON_PLAYING;

              /* Inform the character of a note in progress and the
                possbility of continuation! */
                if (ch->pcdata->in_progress)
                    send_to_char ("You have a note in progress. Type 'note write' to continue it.\n\r",ch);
            }
            return TRUE;
        }
    }

    return FALSE;
}



/*
 * Check if already playing.
 */
bool check_playing( DESCRIPTOR_DATA *d, char *name )
{
    DESCRIPTOR_DATA *dold;

    for ( dold = descriptor_list; dold; dold = dold->next )
    {
        if ( dold != d
        &&   dold->character != NULL
        &&   dold->connected != CON_GET_NAME
        &&   dold->connected != CON_GET_OLD_PASSWORD
        &&   !str_cmp( name, dold->original
                 ? dold->original->name : dold->character->name ) )
        {
            write_to_buffer( d, "That character is already playing.\n\r",0);
        /* added by Rahl */
            write_to_buffer( d, "Do you want to connect anyways? (Y/N) ",0 );
            d->connected = CON_BREAK_CONNECT;
            return TRUE;
        }
    }

    return FALSE;
}



void stop_idling( CHAR_DATA *ch )
{
    if ( ch == NULL
    ||   ch->desc == NULL
    ||   ch->desc->connected != CON_PLAYING
    ||   ch->was_in_room == NULL
    ||   ch->in_room != get_room_index(ROOM_VNUM_LIMBO))
        return;

    ch->timer = 0;
    char_from_room( ch );
    char_to_room( ch, ch->was_in_room );
    ch->was_in_room     = NULL;
    act( "$n has returned from the void.", ch, NULL, NULL, TO_ROOM );
    return;
}



/*
 * Write to one char.
 */
void send_to_char( const char *txt, CHAR_DATA *ch )
{
    if ( txt != NULL && ch->desc != NULL )
        write_to_buffer( ch->desc, txt, strlen(txt) );
    return;
}

void send_to_desc( char *txt, DESCRIPTOR_DATA *d )
{
    if ( txt != NULL && d != NULL ) {
		if ( d->ansi ) {
        	write_to_descriptor( d->descriptor, txt, strlen(txt), 
				TRUE );
		} else {
        	write_to_descriptor( d->descriptor, txt, strlen(txt), 
				FALSE );
		}
	}
		
    return;
}

/*
 * Send a page to one char.
 */
void page_to_char( const char *txt, CHAR_DATA *ch )
{
    if ( txt == NULL || ch->desc == NULL)
        return;

/* if added by Rahl */
    if ( ch->desc == NULL ) return;
    ch->desc->showstr_head = malloc(strlen(txt) + 1);
    strcpy(ch->desc->showstr_head,txt);
    ch->desc->showstr_point = ch->desc->showstr_head;
    show_string(ch->desc,"");
}


/* string pager */
void show_string(struct descriptor_data *d, char *input)
{
    char buffer[4*MAX_STRING_LENGTH];
    char buf[MAX_INPUT_LENGTH];
    register char *scan, *chk;
    int lines = 0, toggle = 1;
    int show_lines;

    one_argument(input,buf);
    if (buf[0] != '\0')
    {
        if (d->showstr_head)
        {
            free_string(d->showstr_head);
            d->showstr_head = 0;
        }
        d->showstr_point  = 0;
        return;
    }

    if (d->character)
        show_lines = d->character->lines;
    else
        show_lines = 0;

    for (scan = buffer; ; scan++, d->showstr_point++)
    {
        if (((*scan = *d->showstr_point) == '\n' || *scan == '\r')
            && (toggle = -toggle) < 0)
            lines++;

        else if (!*scan || (show_lines > 0 && lines >= show_lines))
        {
            *scan = '\0';
            write_to_buffer(d,buffer,strlen(buffer));
            for (chk = d->showstr_point; isspace(*chk); chk++);
            {
                if (!*chk)
                {
                    if (d->showstr_head)
                    {
                        free_string(d->showstr_head);
                        d->showstr_head = 0;
                    }
                    d->showstr_point  = 0;
                }
            }
            return;
        }
    }
    return;
}


/* quick sex fixer */
void fix_sex(CHAR_DATA *ch)
{
    if (ch->sex < 0 || ch->sex > 2)
        ch->sex = IS_NPC(ch) ? 0 : ch->pcdata->true_sex;
}

void act (const char *format, CHAR_DATA *ch, const void *arg1, const void *arg2,
          int type)
{
    /* to be compatible with older code */
    act_new(format,ch,arg1,arg2,type,POS_RESTING);
}

void act_new( const char *format, CHAR_DATA *ch, const void *arg1,
              const void *arg2, int type, int min_pos)
{
    static char * const he_she  [] = { "it",  "he",  "she" };
    static char * const him_her [] = { "it",  "him", "her" };
    static char * const his_her [] = { "its", "his", "her" };

    char buf[MAX_STRING_LENGTH*2];
    char fname[MAX_INPUT_LENGTH];
    CHAR_DATA *to;
    CHAR_DATA *vch = (CHAR_DATA *) arg2;
    OBJ_DATA *obj1 = (OBJ_DATA  *) arg1;
    OBJ_DATA *obj2 = (OBJ_DATA  *) arg2;
    const char *str;
    const char *i;
    char *point;


    buf[0] = '\0';

    /*
     * Discard null and zero-length messages.
     */
    if ( format == NULL || format[0] == '\0' )
        return;

    /* discard null rooms and chars */
    if (ch == NULL || ch->in_room == NULL)
        return;

    to = ch->in_room->people;
    if ( type == TO_VICT )
    {
        if ( vch == NULL )
        {
            bug( "Act: null vch with TO_VICT.", 0 );
            return;
        }

        if (vch->in_room == NULL)
            return;

        to = vch->in_room->people;
    }

    for ( ; to != NULL; to = to->next_in_room )
    {
        if ( to->desc == NULL || to->position < min_pos )
            continue;

        if ( type == TO_CHAR && to != ch )
            continue;
        if ( type == TO_VICT && ( to != vch || to == ch ) )
            continue;
        if ( type == TO_ROOM && to == ch )
            continue;
        if ( type == TO_NOTVICT && (to == ch || to == vch) )
            continue;

        point   = buf;
        str     = format;
        while ( *str != '\0' )
        {
            if ( *str != '$' )
            {
                *point++ = *str++;
                continue;
            }
            ++str;

            if ( arg2 == NULL && *str >= 'A' && *str <= 'Z' )
            {
                bug( "Act: missing arg2 for code %d.", *str );
                i = " <@@@> ";
            }
            else
            {
                switch ( *str )
                {
                default:  bug( "Act: bad code %d.", *str );
                          i = " <@@@> ";                                break;
                /* Thx alex for 't' idea */
                case 't': i = (char *) arg1;                            break;
                case 'T': i = (char *) arg2;                            break;
                case 'n': i = PERS( ch,  to  );                         break;
                case 'N': i = PERS( vch, to  );                         break;
                case 'e': i = he_she  [URANGE(0, ch  ->sex, 2)];        break;
                case 'E': i = he_she  [URANGE(0, vch ->sex, 2)];        break;
                case 'm': i = him_her [URANGE(0, ch  ->sex, 2)];        break;
                case 'M': i = him_her [URANGE(0, vch ->sex, 2)];        break;
                case 's': i = his_her [URANGE(0, ch  ->sex, 2)];        break;
                case 'S': i = his_her [URANGE(0, vch ->sex, 2)];        break;

                case 'p':
                    i = can_see_obj( to, obj1 )
                            ? obj1->short_descr
                            : "something";
                    break;

                case 'P':
                    i = can_see_obj( to, obj2 )
                            ? obj2->short_descr
                            : "something";
                    break;

                case 'd':
                    if ( arg2 == NULL || ((char *) arg2)[0] == '\0' )
                    {
                        i = "door";
                    }
                    else
                    {
                        one_argument( (char *) arg2, fname );
                        i = fname;
                    }
                    break;
                }
            }

            ++str;
            while ( ( *point = *i ) != '\0' )
                ++point, ++i;
        }

        *point++ = '`';
        *point++ = 'w';
        *point++ = '\n';
        *point++ = '\r';
        *point++ = '\0'; 
        buf[0]   = UPPER(buf[0]);
            if (to->desc && (to->desc->connected == CON_PLAYING))
               // write_to_buffer( to->desc, buf, point - buf );
               write_to_buffer( to->desc, buf, 0 );

    }
    return;
}

char *do_color(char *plaintext, bool color)
{
    static char color_text[MAX_STRING_LENGTH*2];
    char *ct_point;

    bzero(color_text, sizeof(color_text));
    ct_point=color_text;

    while ( *plaintext != '\0' )
    {
        if ( *plaintext != '`' )
        {
            *ct_point = *plaintext;
            ct_point++;
            plaintext++;
            continue;
        }
        plaintext++;

        if (!color)
            switch(*plaintext)
            {
                case 'k':
                case 'K':
                case 'r':
                case 'R':
                case 'b':
                case 'B':
                case 'c':
                case 'C':
                case 'Y':
                case 'y':
                case 'm':
                case 'M':
                case 'w':
                case 'W':
                case 'g':
                case 'G':
                    plaintext++;
                    break;
                default:
                    strcat(color_text, "`");
                    ct_point++;
                    break;
            }   /* switch */
            else
                switch(*plaintext)
                {
                    case 'k':
                        strcat(color_text, "[0;30m");
                        ct_point+=7;
                        plaintext++;
                        break;
                    case 'K':
                        strcat(color_text, "[1;30m");
                        ct_point+=7;
                        plaintext++;
                        break;
                    case 'r':
                        strcat(color_text, "[0;31m");
                        ct_point+=7;
                        plaintext++;
                        break;
                    case 'R':
                        strcat(color_text, "[1;31m");
                        ct_point+=7;
                        plaintext++;
                        break;
                    case 'g':
                        strcat(color_text, "[0;32m");
                        ct_point+=7;
                        plaintext++;
                        break;
                    case 'G':
                        strcat(color_text, "[1;32m");
                        ct_point+=7;
                        plaintext++;
                        break;
                    case 'y':
                        strcat(color_text, "[0;33m");
                        ct_point+=7;
                        plaintext++;
                        break;
                    case 'Y':
                        strcat(color_text, "[1;33m");
                        ct_point+=7;
                        plaintext++;
                        break;
                    case 'b':
                        strcat(color_text, "[0;34m");
                        ct_point+=7;
                        plaintext++;
                        break;
                    case 'B':
                        strcat(color_text, "[1;34m");
                        ct_point+=7;
                        plaintext++;
                        break;
                    case 'm':
                        strcat(color_text, "[0;35m");
                        ct_point+=7;
                        plaintext++;
                        break;
                    case 'M':
                        strcat(color_text, "[1;35m");
                        ct_point+=7;
                        plaintext++;
                        break;
                    case 'c':
                        strcat(color_text, "[0;36m");
                        ct_point+=7;
                        plaintext++;
                        break;
                    case 'C':
                        strcat(color_text, "[1;36m");
                        ct_point+=7;
                        plaintext++;
                        break;
                    case 'w':
                        strcat(color_text, "[0;37m");
                        ct_point+=7;
                        plaintext++;
                        break;
                    case 'W':
                        strcat(color_text, "[1;37m");
                        ct_point+=7;
                        plaintext++;
                        break;
                    default:
                        strcat(color_text, "`");
                        ct_point++;
                        break;
                } /* switch */
    }
    strcat(color_text, "[0m\0");
    return(color_text);
}

char *figurestate(int current, int max)
{
    static char status[40];

    bzero(status, sizeof(status));
    if (current >= (max/3*2)) sprintf(status, "`W%d`w", current);
    else if (current >= max/3) sprintf(status, "`Y%d`w", current);
    else sprintf(status, "`R%d`w", current);
    return (status);
}

char *damstatus(CHAR_DATA *ch)
{
    int percent;
    static char wound[40];

    bzero(wound, sizeof(wound));
    if (ch->max_hit > 0)
        percent = ch->hit * 100 / ch->max_hit;
    else
        percent = -1;
    if (percent >= 100)
        sprintf(wound,"excellent condition");
    else if (percent >= 90)
        sprintf(wound,"few scratches");
    else if (percent >= 75)
        sprintf(wound,"small wounds");
    else if (percent >= 50)
        sprintf(wound,"quite a few wounds");
    else if (percent >= 30)
        sprintf(wound,"nasty wounds");
    else if (percent >= 15)
        sprintf(wound,"pretty hurt");
    else if (percent >= 0)
        sprintf(wound,"awful condition");
    else
        sprintf(wound,"bleeding to death");
    return (wound);
}

char *doparseprompt(CHAR_DATA *ch)
{
    CHAR_DATA *tank,*victim;
    static char finished_prompt[240];
    char workstr[100];
    char *fp_point;
    char *orig_prompt;
	char doors[MAX_INPUT_LENGTH];
	EXIT_DATA *pexit;
	bool found;
	const char *dir_name[] = {"N", "E", "S", "W", "U", "D"};
	int door;

    bzero(finished_prompt, sizeof(finished_prompt));
    orig_prompt=ch->pcdata->prompt;
    fp_point=finished_prompt;

    while(*orig_prompt != '\0')
    {
        if (*orig_prompt != '%')
        {
            *fp_point = *orig_prompt;
            orig_prompt++;
            fp_point++;
            continue;
        }
        orig_prompt++;
        switch(*orig_prompt)
        {
            case 'h':
                sprintf(workstr, "%d", ch->hit);
                strcat(finished_prompt, workstr);
                fp_point+=strlen(workstr);
                orig_prompt++;
                break;
            case 'H':
                sprintf(workstr, "%d", ch->max_hit);
                strcat(finished_prompt, workstr);
                fp_point+=strlen(workstr);
                orig_prompt++;
                break;
            case 'm':
                sprintf(workstr, "%d", ch->mana);
                strcat(finished_prompt, workstr);
                fp_point+=strlen(workstr);
                orig_prompt++;
                break;
            case 'M':
                sprintf(workstr, "%d", ch->max_mana);
                strcat(finished_prompt, workstr);
                fp_point+=strlen(workstr);
                orig_prompt++;
                break;
            case 'v':
                sprintf(workstr, "%d", ch->move);
                strcat(finished_prompt, workstr);
                fp_point+=strlen(workstr);
                orig_prompt++;
                break;
            case 'V':
                sprintf(workstr, "%d", ch->max_move);
                strcat(finished_prompt, workstr);
                fp_point+=strlen(workstr);
                orig_prompt++;
                break;
            case 'r':
                strcat(finished_prompt, "\n\r");
                fp_point++;
                orig_prompt++;
                break;
            case 'i':
                sprintf(workstr, "%s", figurestate(ch->hit, ch->max_hit));
                strcat(finished_prompt, workstr);
                fp_point+=strlen(workstr);
                orig_prompt++;
                break;
            case 'n':
                sprintf(workstr, "%s", figurestate(ch->mana, ch->max_mana));
                strcat(finished_prompt, workstr);
                fp_point+=strlen(workstr);
                orig_prompt++;
                break;
            case 'w':
                sprintf(workstr, "%s", figurestate(ch->move, ch->max_move));
                strcat(finished_prompt, workstr);
                fp_point+=strlen(workstr);
                orig_prompt++;
                break;
            case 'l':
                if ((tank=ch->fighting) != NULL)
                    if ((tank=tank->fighting) != NULL)
                    {
                        sprintf(workstr, "%s", damstatus(tank));
                        strcat(finished_prompt, workstr);
                        fp_point+=strlen(workstr);
                    }
                orig_prompt++;
                break;
            case 'e':
                if ((victim=ch->fighting) != NULL)
                {
                    sprintf(workstr, "%s", damstatus(victim));
                    strcat(finished_prompt, workstr);
                    fp_point+=strlen(workstr);
                }
                orig_prompt++;
                break;
            case 's':
                sprintf(workstr, "%s", damstatus(ch));
                strcat(finished_prompt, workstr);
                fp_point+=strlen(workstr);
                orig_prompt++;
                break;
            case 't':
                sprintf(workstr, "%d %s", (time_info.hour % 12 == 0)
                    ? 12 : time_info.hour % 12, time_info.hour >=
                    12 ? "pm" : "am");
                strcat(finished_prompt, workstr);
                fp_point+=strlen(workstr);
                orig_prompt++;
                break;
            case 'T':
                /* sprintf(workstr, "%d", time_info.hour); */
                sprintf(workstr, "%s", get_curtime() );
                strcat(finished_prompt, workstr);
                fp_point+=strlen(workstr);
                orig_prompt++;
                break;
            case '#':
                if( IS_IMMORTAL( ch ) && ch->in_room != NULL )
                {
                    sprintf(workstr, "%d", ch->in_room->vnum);
                    strcat(finished_prompt, workstr);
                    fp_point+=strlen(workstr);
                    orig_prompt++;
                    break;
                }
                else
                    break;
            case 'A':
                if (IS_SET(ch->act,PLR_AFK))
                {
                    sprintf(workstr, "`W(AFK)");
                    strcat(finished_prompt, workstr);
                    fp_point+=strlen(workstr);
                }
                if (IS_SET(ch->act,PLR_KILLER))
                {
                    sprintf(workstr, "`R(PK)");
                    strcat(finished_prompt, workstr);
                    fp_point+=strlen(workstr);
                }
                if (IS_SET(ch->act,PLR_THIEF))
                {
                    sprintf(workstr, "`K(THIEF)");
                    strcat(finished_prompt, workstr);
                    fp_point+=strlen(workstr);
                }
                /* quiet by Rahl */
                if( IS_SET( ch->comm, COMM_QUIET))
                {
                    sprintf(workstr, "`Y(QUIET)");
                    strcat(finished_prompt, workstr);
                    fp_point+=strlen(workstr);
                }
                if( IS_IMMORTAL( ch ) && IS_SET(ch->act, PLR_WIZINVIS))
                {
                    sprintf(workstr, "`B(Wizi:%d)",ch->invis_level);
                    strcat(finished_prompt, workstr);
                    fp_point+=strlen(workstr);
                }
                /* incog by Rahl */
                if( IS_IMMORTAL( ch ) && IS_SET(ch->act, PLR_INCOGNITO ) )
                {
                    sprintf(workstr, "`C(Incog:%d)",ch->incog_level );
                    strcat( finished_prompt, workstr);
                    fp_point+=strlen(workstr);
                }
                orig_prompt++;
                break;
            case 'x':
                sprintf(workstr, "%ld", ch->exp);
                strcat(finished_prompt, workstr);
                fp_point+=strlen(workstr);
                orig_prompt++;
                break;
            case 'X':
                sprintf(workstr, "%ld",
                    exp_per_level(ch,ch->pcdata->points)-ch->exp);
                strcat(finished_prompt, workstr);
                fp_point+=strlen(workstr);
                orig_prompt++;
                break;
            /* date case by Rahl */
            case 'd':
                sprintf(workstr, "%s", get_curdate() );
                strcat(finished_prompt, workstr);
                fp_point+=strlen(workstr);
                orig_prompt++;
                break;
			case 'g':
				sprintf( workstr, "%ld", ch->gold );
				strcat( finished_prompt, workstr );
				fp_point += strlen( workstr );
				orig_prompt++;
				break;
			case 'a':
				sprintf( workstr, "%d", ch->alignment );
				strcat( finished_prompt, workstr );
				fp_point += strlen( workstr );
				orig_prompt++;
				break;
			case 'E':
				found = FALSE;
				doors[0] = '\0';
				for ( door = 0; door < 6; door++ ) {
					if ( ( pexit = ch->in_room->exit[door] ) != NULL
					&& pexit->u1.to_room != NULL
					&& ( can_see_room( ch, pexit->u1.to_room )
					|| ( IS_AFFECTED( ch, AFF_DARK_VISION )
					&& !IS_AFFECTED( ch, AFF_BLIND ) ) )
					&& !IS_SET( pexit->exit_info, EX_CLOSED ) ) {
						found = TRUE;
						strcat( doors, dir_name[door] );
					}
				}
				if ( !found ) {
					sprintf( workstr, "none" );
				} else {
					sprintf( workstr, "%s", doors );
				}
				strcat( finished_prompt, workstr );
				fp_point += strlen( workstr );
				orig_prompt++;
				break;
            default:
                strcat(finished_prompt, "%");
                fp_point++;
                break;
        }
    }
    return(finished_prompt);
}

/*
 *  Copyover - Original idea: Fusion of MUD
 *  Adapted to Diku by Erwin S. Andreasen, <erwin@pip.dknet.dk>
 *  http://pip.dknet.dk/~pip1773
 *  Changed into a ROM patch after seeing the 100th request for it :)
 */
void do_copyover (CHAR_DATA *ch, char * argument) {
    FILE *fp;
    DESCRIPTOR_DATA *d, *d_next;
    char buf [100], buf2[100];
    extern int port,control; /* db.c */
    char log_buf[MAX_STRING_LENGTH];

    if ( chaos ) {
        send_to_char( "You must wait until "
            "`rC`RH`YA`RO`rs`w has ended.\n\r", ch );
        return;
    }

    if ( IS_NPC( ch ) ) {
        send_to_char( "Try again when you aren't switched.\n\r", ch );
        return;
    }

    fp = fopen (COPYOVER_FILE, "w");

    if (!fp) {
        send_to_char ("\n\rCopyover file not writeable, aborted.\n\r",ch);
        sprintf (log_buf, "Could not write to copyover file: %s",
            COPYOVER_FILE);
        log_string( log_buf );
        perror ("do_copyover:fopen");
        return;
    }

    /* Consider changing all saved areas here, if you use OLC */

    do_asave (ch, "changed"); /* - autosave changed areas */
    do_force( ch, "all save" );
    do_save( ch, "" );

    sprintf (buf, "\n\r`wWith a loud rumble the world shakes and trembles"
        " before exploding into a cloud \nof dust.\n\r`w" );

    /* For each playing descriptor, save its state */
    for (d = descriptor_list; d ; d = d_next) {
        CHAR_DATA * och = CH (d);

        d_next = d->next; /* We delete from the list , so need to save this */

        /* drop those logging on */
        if (!d->character || d->connected > CON_PLAYING ) {
            write_to_descriptor (d->descriptor, "\n\rSorry, we are"
                " rebooting. Come back in a few minutes.\n\r", 0, TRUE);
            close_socket (d); /* throw'em out */
        } else {
            fprintf (fp, "%d %s %s\n", d->descriptor, och->name, d->host);
            /* This is not necessary for ROM */
        /*
            if (och->level == 1) {
                write_to_descriptor(d->descriptor,
                    "Since you are level one, and level one characters do"
                    " not save, you gain a free level!\n\r", 0, TRUE);
                advance_level (och);
                och->level++;
            }
        */

            save_char_obj (och);

            write_to_descriptor (d->descriptor, buf, 0, TRUE);
        }
    }

    fprintf (fp, "-1\n");
    fclose (fp);

    /*
     * Close reserve and other always-open files and release other
     * resources
     */

    fclose (fpReserve);

    /* exec - descriptors are inherited */

    sprintf (buf, "%d", port);
    sprintf (buf2, "%d", control);
    execl (EXE_FILE, "shadows", buf, "copyover", buf2, (char *) NULL);

    /* Failed - sucessful exec will not return */

    perror ("do_copyover: execl");
    send_to_char ("Copyover FAILED!\n\r",ch);

     /* Here you might want to reopen fpReserve */
     fpReserve = fopen (NULL_FILE, "r");
}

/* Recover from a copyover - load players */
void copyover_recover( void )
{
    DESCRIPTOR_DATA *d;
    FILE *fp;
    char name [100];
    char host[MAX_STRING_LENGTH];
    int desc;
    bool fOld;
    /* char log_buf[MAX_STRING_LENGTH]; */

    sprintf (log_buf, "Copyover recovery initiated");
    log_string( log_buf );

    fp = fopen (COPYOVER_FILE, "r");

    /* there are some descriptors open which will hang forever then ? */
    if (!fp) {
        perror ("copyover_recover:fopen");
        sprintf (log_buf, "Copyover file not found. Exitting.\n\r");
        log_string( log_buf );
        exit (1);
    }

    /* In case something crashes - doesn't prevent reading      */
    unlink (COPYOVER_FILE);

    for (;;) {
        fscanf (fp, "%d %s %s\n", &desc, name, host);
        if (desc == -1)
            break;

        /* Write something, and check if it goes error-free */
        if (!write_to_descriptor (desc, "\n\rWith a loud rush, the world"
            " reforms, bringing with it new life.\n\r",0, TRUE))
        {
            close (desc); /* nope */
            continue;
        }

        d = alloc_perm (sizeof(DESCRIPTOR_DATA));
        init_descriptor(d,desc);
        d->host = str_dup (host);
        d->next = descriptor_list;
        descriptor_list = d;

        /* -15, so close_socket frees the char */
        d->connected = CON_COPYOVER_RECOVER;


        /* Now, find the pfile */

        fOld = load_char_obj (d, name);

        if (!fOld) /* Player file not found?! */ {
            write_to_descriptor (desc, "\n\rSomehow, your character was"
                " lost in the copyover. Please see an immortal.\n\r",
                0, TRUE);
            close_socket (d);
        } else /* ok! */ {
            /*
             write_to_descriptor (desc, "\n\rCopyover recovery"
                " complete.\n\r",0, TRUE);
            */

            /* Just In Case */
            if (!d->character->in_room)
                d->character->in_room = get_room_index (ROOM_VNUM_TEMPLE);

            /* Insert in the char_list */
            d->character->next = char_list;
            char_list = d->character;

            char_to_room (d->character, d->character->in_room);
            do_look (d->character, "auto");
            act ("$n materializes!", d->character, NULL, NULL, TO_ROOM);
            d->connected = CON_PLAYING;
			/* do I need to add a reset_char( d->character ) here? */

            if (d->character->pet != NULL) {
                char_to_room(d->character->pet,d->character->in_room);
                act("$n materializes!", d->character->pet, NULL, NULL,
                    TO_ROOM);
            }
        }
    }
	fclose( fp );
}


