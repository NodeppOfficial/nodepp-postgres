#ifndef NODEPP_POSTGRES
#define NODEPP_POSTGRES

/*────────────────────────────────────────────────────────────────────────────*/

#include <postgresql/libpq-fe.h>
#include <nodepp/nodepp.h>
#include <nodepp/url.h>

/*────────────────────────────────────────────────────────────────────────────*/

#ifndef NODEPP_POSTGRES_GENERATOR
#define NODEPP_POSTGRES_GENERATOR

namespace nodepp { namespace _postgres_ { GENERATOR(cb) {
protected:

    int x, y, num_row, num_col;
    array_t<string_t> col;

public:

    template< class T, class U, class V, class Q > coEmit( T& fd, U& res, V& cb, Q& self ){
    coBegin ; coWait( self->is_used()==1 ); self->use();

        num_col = PQntuples(res);
        num_row = PQnfields(res);

        for( x=0; x<num_col; x++ )
           { col.push( PQgetvalue( res, x, 0 ) ); }

        for( y=1; y<num_row; y++ ){ do {
        auto object = map_t<string_t,string_t>();
        for( x=0; x<num_col; x++ ){
             auto row = PQgetvalue( res, x, y );
             object[ col[y] ] = row ? row : "NULL";
        }    cb( object ); } while(0); coNext; }

        self->release(); PQclear(res);

    coFinish
    }

};}}

#endif

/*────────────────────────────────────────────────────────────────────────────*/

namespace nodepp { using sql_item_t = map_t<string_t,string_t>; }

namespace nodepp { class postgres_t {
protected:

    struct NODE {
        PGconn *fd = nullptr;
        bool  used = 0;
        int  state = 1;
    };  ptr_t<NODE> obj;

public:

    event_t<> onUse;
    event_t<> onRelease;

    /*─······································································─*/

    virtual ~postgres_t() noexcept {
        if( obj.count() > 1 || obj->fd == nullptr ){ return; }
        if( obj->state == 0 ){ return; } free();
    }

#ifdef NODEPP_SSL
    postgres_t ( string_t uri, string_t name, ssl_t* ssl ) : obj( new NODE ) {

        auto host = url::hostname( uri );
        auto user = url::user( uri );
        auto pass = url::pass( uri );
        auto port = url::port( uri );

        obj->fd = PQconnectdb( regex::format( "dbname=${0} host=${1} user=${2} password=${3} port=${4} sslcert=${5} sslkey=${6}",
            name, host, user, pass, port, ssl->get_crt_path(), ssl->get_key_path()
        ).get() ); if( PQstatus( obj->fd ) != CONNECTION_OK ) {
            throw except_t( PQerrorMessage(obj->fd) );
        }

    }
#endif

    /*─······································································─*/

    postgres_t ( string_t uri, string_t name ) : obj( new NODE ) {

        auto host = url::hostname( uri );
        auto user = url::user( uri );
        auto pass = url::pass( uri );
        auto port = url::port( uri );

        obj->fd = PQconnectdb( regex::format( "dbname=${0} host=${1} user=${2} password=${3} port=${4}",
            name, host, user, pass, port
        ).get() ); if( PQstatus( obj->fd ) != CONNECTION_OK ) {
            throw except_t( PQerrorMessage(obj->fd) );
        }

    }

    postgres_t () : obj( new NODE ) { obj->state = 0; }

    /*─······································································─*/

    array_t<sql_item_t> exec( const string_t& cmd ) const { queue_t<sql_item_t> arr;
        function_t<void,sql_item_t> cb = [&]( sql_item_t args ){ arr.push( args ); };
        if( cmd.empty() || obj->state==0 ){ return nullptr; }

        PGresult *res = PQexec( obj->fd, cmd.data() );

        if ( PQresultStatus(res) != PGRES_TUPLES_OK ) { PQclear(res);
             throw except_t( PQerrorMessage(obj->fd) ); return nullptr;
        }

        auto self = type::bind( this ); _postgres_::cb task;
        process::await( task, obj->fd, res, cb, self ); return arr.data();
    }

    void exec( const string_t& cmd, const function_t<void,sql_item_t>& cb ) const {
        if( cmd.empty() || obj->state==0 ){ return; }

        PGresult *res = PQexec( obj->fd, cmd.data() );

        if ( PQresultStatus(res) != PGRES_TUPLES_OK ) { PQclear(res);
             throw except_t( PQerrorMessage(obj->fd) ); return;
        }

        auto self = type::bind( this ); _postgres_::cb task;
        process::add( task, obj->fd, res, cb, self );
    }

    /*─······································································─*/

    void async( const string_t& cmd ) const {
        function_t<void,sql_item_t> cb = [=]( sql_item_t ){};
        if( cmd.empty() || obj->state==0 ){ return; }

        PGresult *res = PQexec( obj->fd, cmd.data() );

        if ( PQresultStatus(res) != PGRES_TUPLES_OK ) { PQclear(res);
             throw except_t( PQerrorMessage(obj->fd) ); return;
        }

        auto self = type::bind( this ); _postgres_::cb task;
        process::add( task, obj->fd, res, cb, self );
    }

    void await( const string_t& cmd ) const {
        function_t<void,sql_item_t> cb = [&]( sql_item_t ){};
        if( cmd.empty() || obj->state==0 ){ return; }

        PGresult *res = PQexec( obj->fd, cmd.data() );

        if ( PQresultStatus(res) != PGRES_TUPLES_OK ) { PQclear(res);
             throw except_t( PQerrorMessage(obj->fd) ); return;
        }

        auto self = type::bind( this ); _postgres_::cb task;
        process::await( task, obj->fd, res, cb, self ); return arr.data();
    }

    /*─······································································─*/

    void use()     const noexcept { if( obj->used==1 ){ return; } obj->used=1; onUse    .emit(); }
    void release() const noexcept { if( obj->used==0 ){ return; } obj->used=0; onRelease.emit(); }
    bool is_used() const noexcept { return obj->used; }

    /*─······································································─*/

    void free() const noexcept {
        if( obj->fd == nullptr ){ return; }
        if( obj->state == 0 )   { return; }
        PQfinish( obj->fd );
        release(); obj->state = 0;
    }

};}

/*────────────────────────────────────────────────────────────────────────────*/

#endif
