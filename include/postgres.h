#ifndef NODEPP_SQLITE
#define NODEPP_SQLITE

/*────────────────────────────────────────────────────────────────────────────*/

#include <nodepp/nodepp.h>
#include <nodepp/url.h>
#include <nodepp/ssl.h>
#include <libpq-fe.h>

namespace nodepp { using sql_item_t = map_t<string_t,string_t>; }

namespace nodepp { class postgres_t {
protected:

    struct NODE {
        PGconn *fd;
        int state = 1;
    };  ptr_t<NODE> obj;

    template< class T > void callback( T& cb, PGresult* res ) { 
        sql_item_t arguments; array_t<string_t> col;

        if( result == NULL ) {
            string_t message = mysql_error( obj->fd );
            process::error( message );
        }

        int num_row = PQntuples( res );
        int num_col = PQnfields( res );

        for( int x=0; x<num_row; x++ )
           { col.push( PQgetvalue(res,x,0) ); }

        for( int y=1; y<num_col; y++ ){
        for( int x=0; x<num_row; x++ ){
             auto data = PQgetvalue( res, x, y );
             arguments[ col[y] ] = data ? data : "NULL"; 
        } cb(arguments); }

        PQclear(res);
    }

public:
    
    virtual ~postgres_t() noexcept {
        if( obj.count() > 1 || obj->fd == nullptr ){ return; }
        if( obj->state == 0 ){ return; } free();
    }
    
    /*─······································································─*/

    virtual void free() const noexcept {
        if( obj->fd == nullptr ){ return; }
        if( obj->state == 0 )   { return; }
        PQfinish( obj->fd );
        obj->state = 0; 
    }
    
    /*─······································································─*/
    
    postgres_t ( string_t uri, string_t name, ssl_t* ssl ) : obj( new NODE ) {

        auto host = url::hostname( uri );
        auto user = url::user( uri );
        auto pass = url::pass( uri );
        auto port = url::port( uri );

        char* key = ssl->get_key_path()==nullptr ? nullptr : ssl->get_key_path().get();
        char* crt = ssl->get_crt_path()==nullptr ? nullptr : ssl->get_crt_path().get();

        obj->fd = PQconnectdb( regex::format( "dbname=${0} user=${1} password=${2} port=${3} sslcert=${4} sslkey=${5}",
            name, user, pass, port, crt, key
        ).get() ); if( obj->fd == nullptr ) {
            process::error("Error: Can't Start MySQL"); 
        }

    }
    
    /*─······································································─*/
    
    postgres_t ( string_t uri, string_t name ) : obj( new NODE ) {

        auto host = url::hostname( uri );
        auto user = url::user( uri );
        auto pass = url::pass( uri );
        auto port = url::port( uri );

        obj->fd = PQconnectdb( regex::format( "dbname=${0} user=${1} password=${2} port=${3}",
            name, user, pass, port
        ).get() ); if( obj->fd == nullptr ) { 
            process::error("Error: Can't Start MySQL"); 
        }

    }
    
    /*─······································································─*/

    void exec( const string_t& cmd, const function_t<void,sql_item_t>& cb ) {
        PGresult *res = PQexec( obj->fd, cmd.data() );
        if ( PQresultStatus(res) != PGRES_TUPLES_OK ) {
             process::error( PQerrorMessage(obj->fd) );
             PQclear(res); return;
        }    callback( cb, res );
    }

    void exec( const string_t& cmd ) {
        PGresult *res = PQexec( obj->fd, cmd.data() );
        if ( PQresultStatus(res) != PGRES_TUPLES_OK ) {
             process::error( PQerrorMessage(obj->fd) );
             PQclear(res); return;
        }    PQclear(res);
    }

};}

/*────────────────────────────────────────────────────────────────────────────*/

#endif
