/*
 * Copyright 2023 The Nodepp Project Authors. All Rights Reserved.
 *
 * Licensed under the MIT (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://github.com/NodeppOfficial/nodepp/blob/main/LICENSE
 */

/*────────────────────────────────────────────────────────────────────────────*/

#ifndef NODEPP_POSTGRES
#define NODEPP_POSTGRES

/*────────────────────────────────────────────────────────────────────────────*/

#include <nodepp/nodepp.h>
#include <nodepp/expected.h>
#include <nodepp/optional.h>
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
        if( cb.null() ){ PQclear(ctx); self->release(); coEnd; }
        
        num_cols = PQnfields(ctx);
        num_rows = PQntuples(ctx);

        for( c=0; c<num_cols; c++ )
           { col.push( PQfname( ctx, c ) ); }

        for( r=0; r<num_rows &&; r++ ){ do {
        auto object = map_t<string_t,string_t>();

        for( c=0; c<num_cols; c++ ){
             auto value = PQgetvalue( ctx, r, c );
             object[ col[c] ] = value ? value : "NULL";
        }    cb( object ); } while(0); coNext; }

        PQclear(ctx); self->release(); 

    coFinish }

};}}

#endif

/*────────────────────────────────────────────────────────────────────────────*/

#ifndef NODEPP_SQL_ITEM
#define NODEPP_SQL_ITEM
namespace nodepp { using sql_item_t = map_t<string_t,string_t>; }
#endif

/*────────────────────────────────────────────────────────────────────────────*/
namespace nodepp { class postgres_t {
protected:

    enum STATE {
        SQL_STATE_UNKNOWN = 0b00000000,
        SQL_STATE_OPEN    = 0b00000001,
        SQL_STATE_USED    = 0b10000000,
        SQL_STATE_CLOSE   = 0b00000010
    };

    struct NODE {
        PGconn *fd  = nullptr;
        int state   = 0;
    };  ptr_t<NODE> obj;

public:

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

        obj->state = SQL_STATE_OPEN;

    }
#endif

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

        obj->state = SQL_STATE_OPEN;

    }

   ~postgres_t () noexcept { if( obj.count() > 1 ){ return; } free(); }

    postgres_t () : obj( new NODE ) { obj->state = STATE::SQL_STATE_CLOSE; }

    /*─······································································─*/

    promise_t<array_t<sql_item_t>,except_t> resolve( const string_t& cmd ) const { 
           queue_t<sql_item_t> arr; auto self = type::bind( this );
    return promise_t<array_t<sql_item_t>,except_t>([=]( 
        res_t<array_t<sql_item_t>> res, 
        rej_t<except_t> /*------*/ rej 
    ){

        function_t<void,sql_item_t> cb ([=]( sql_item_t args ){ arr.push(args); });
        
        if( cmd.empty() || self->is_closed() || self->obj->fd==nullptr )
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

    expected_t<array_t<sql_item_t>,except_t>
    await( const string_t& cmd ) const { return resolve( cmd ).await(); }

    /*─······································································─*/

    optional_t<except_t>
    emit( const string_t& cmd, function_t<void,sql_item_t> cb=nullptr ) const {

        if( cmd.empty() || is_closed() || obj->fd==nullptr )
          { return except_t( "Postgres Error: Closed" ); }

        PGresult *ctx = PQexec( obj->fd, cmd.data() );

        if( PQresultStatus(ctx) != PGRES_TUPLES_OK  ) {
        if( PQresultStatus(ctx) != PGRES_COMMAND_OK ) { PQclear(ctx); 
            return except_t( PQerrorMessage(obj->fd) );
        }   PQclear(ctx); return nullptr; }

        auto self = type::bind( this ); _postgres_::cb task;
        process::add( task, obj->fd, ctx, cb, self );

    return nullptr; }

    /*─······································································─*/

    bool is_closed()    const noexcept { return obj->state & STATE::SQL_STATE_CLOSE; }
    bool is_used()      const noexcept { return obj->state & STATE::SQL_STATE_USED ; }
    void close()        const noexcept { /*--*/ obj->state = STATE::SQL_STATE_CLOSE; }
    void use()          const noexcept { /*--*/ obj->state|= STATE::SQL_STATE_USED ; }
    void release()      const noexcept { /*--*/ obj->state&=~STATE::SQL_STATE_USED ; }
    bool is_available() const noexcept { return !is_closed(); }

    /*─······································································─*/

    void free() const noexcept {
        if( obj->fd == nullptr ){ return; }
        if( is_closed() ) /*-*/ { return; }
        close(); PQfinish( obj->fd );
    }

};}

/*────────────────────────────────────────────────────────────────────────────*/

#endif
