#ifndef NODEPP_POSTGRES
#define NODEPP_POSTGRES

/*────────────────────────────────────────────────────────────────────────────*/

#include <nodepp/nodepp.h>
#include <nodepp/promise.h>
#include <nodepp/url.h>

#include <postgresql/libpq-fe.h>

/*────────────────────────────────────────────────────────────────────────────*/

#ifndef NODEPP_POSTGRES_GENERATOR
#define NODEPP_POSTGRES_GENERATOR

namespace nodepp { namespace _postgres_ { GENERATOR(cb) {
protected:

    int c, r, num_rows, num_cols;
    array_t<string_t> col;

public:

    template< class T, class U, class V, class Q > coEmit( T& fd, U& ctx, V& cb, Q& self ){
    coBegin ; coWait( self->is_used()==1 ); self->use();
        
        num_cols = PQnfields(ctx);
        num_rows = PQntuples(ctx);

        for( c=0; c<num_cols; c++ )
           { col.push( PQfname( ctx, c ) ); }

        for( r=0; r<num_rows; r++ ){ do {
        auto object = map_t<string_t,string_t>();

        for( c=0; c<num_cols; c++ ){
             auto value = PQgetvalue( ctx, r, c );
             object[ col[c] ] = value ? value : "NULL";
        }    cb( object ); } while(0); coNext; }

        self->release(); PQclear(ctx);

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
        bool state = 1;
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

    promise_t<array_t<sql_item_t>,except_t> resolve( const string_t& cmd ) const { 
           queue_t<sql_item_t> arr; auto self = type::bind( this );
    return promise_t<array_t<sql_item_t>,except_t>([=]( res_t<array_t<sql_item_t>> res, rej_t<except_t> rej ){

        function_t<void,sql_item_t> cb ([=]( sql_item_t args ){ arr.push(args); });
        
        if( cmd.empty() || self->is_closed() )
          { rej(except_t( "Postgres Error: Closed" )); return; }

        PGresult *ctx = PQexec( self->obj->fd, cmd.data() );

        if( PQresultStatus(ctx) != PGRES_TUPLES_OK  ) {
        if( PQresultStatus(ctx) != PGRES_COMMAND_OK ) { PQclear(ctx); 
            rej(except_t( PQerrorMessage(self->obj->fd) )); return;
        }   PQclear(ctx); res( nullptr ); return; }

        auto task = type::bind( _postgres_::cb() ); process::add([=](){
            while( (*task)( ctx, cb, self )>=0 ){ return 1; }
            res( arr.data() ); return -1; 
        }); 
    
    }); }

    /*─······································································─*/

    array_t<sql_item_t> await( const string_t& cmd ) const {
        auto data = resolve( cmd ).await();
        if( data.has_value() ){ return data.value(); }
        else /*------------*/ { throw  data.error(); }
    }

    /*─······································································─*/

    void emit( const string_t& cmd, function_t<void,sql_item_t> cb=nullptr ) const {

        if( cmd.empty() || is_closed() )
          { throw except_t( "Postgres Error: Closed" ); }

        PGresult *ctx = PQexec( obj->fd, cmd.data() );

        if( PQresultStatus(ctx) != PGRES_TUPLES_OK  ) {
        if( PQresultStatus(ctx) != PGRES_COMMAND_OK ) { PQclear(ctx); 
            throw except_t( PQerrorMessage(obj->fd) );
        }   PQclear(ctx); return; }

        auto self = type::bind( this ); _postgres_::cb task;
        process::add( task, obj->fd, ctx, cb, self );
    }

    /*─······································································─*/

    void use()       const noexcept { if( obj->used==1 ){ return; } obj->used=1; onUse    .emit(); }
    void release()   const noexcept { if( obj->used==0 ){ return; } obj->used=0; onRelease.emit(); }

    /*─······································································─*/

    bool is_closed() const noexcept { return obj->state==0; }
    bool is_used()   const noexcept { return obj->used; }

    /*─······································································─*/

    void free() const noexcept {
        if( obj->fd == nullptr ){ return; }
        if( obj->state == 0 )   { return; }
        PQfinish( obj->fd ); release(); obj->state = 0;
    }

};}

/*────────────────────────────────────────────────────────────────────────────*/

#endif
