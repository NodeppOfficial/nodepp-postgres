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

namespace nodepp { namespace _postgres_ { GENERATOR( pipe ) {
protected:

    int c, r, num_rows, num_cols;
    array_t<string_t> col;

public:

    template< class U, class V > coEmit( U& ctx, V& cb ){
    coBegin

        if( cb.null() ){ PQclear(ctx); coEnd; }
        
        num_cols = PQnfields(ctx); col.clear();
        num_rows = PQntuples(ctx);

        for( c=0; c<num_cols; c++ )
           { col.push( PQfname( ctx, c ) ); }

        for( r=0; r<num_rows; r++ ){ do {
        auto object = map_t<string_t,string_t>();

        for( c=0; c<num_cols; c++ ){
             auto value = PQgetvalue( ctx, r, c );
             object[ col[c] ] = value ? value : "NULL";
        }    cb( object ); } while(0); coNext; }

        PQclear(ctx);

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
        PGconn *fd=nullptr; /**/ int state=0;
       ~NODE() { if( fd ){ PQfinish( fd ); }}
    };  ptr_t<NODE> obj;

    void use()     const noexcept { obj->state|= STATE::SQL_STATE_USED ; }
    void release() const noexcept { obj->state&=~STATE::SQL_STATE_USED ; }

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

        obj->state = STATE::SQL_STATE_OPEN;

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

        obj->state = STATE::SQL_STATE_OPEN;

    }

   ~postgres_t () noexcept { if( obj.count() > 1 ){ return; } free(); }

    postgres_t () : obj( new NODE ) { obj->state = STATE::SQL_STATE_CLOSE; }

    /*─······································································─*/

    expected_t<postgres_t, except_t> 
    emit( const string_t& cmd, function_t<void,sql_item_t> cb=nullptr ) const noexcept {
    except_t err; do {

        if( is_used() )
          { return except_t( "SQL Error: client is already used" ); }

        if( cmd.empty() || is_closed() || obj->fd==nullptr )
          { err = except_t( "Postgres Error: Closed" ); break; }

        PGresult *ctx = PQexec( obj->fd, cmd.data() );

        if( PQresultStatus(ctx) != PGRES_TUPLES_OK  ) {
        if( PQresultStatus(ctx) != PGRES_COMMAND_OK ) { PQclear(ctx); 
            return except_t( PQerrorMessage(obj->fd) );
        }   PQclear(ctx); release(); return nullptr; }

        _postgres_::pipe pipe; 
        
        while( pipe( ctx, cb )==1 ){ /*unused*/ }

        /*---*/ release(); return *this;    
    } while(0); release(); return  err ; }

    /*─······································································─*/

    expected_t<ptr_t<sql_item_t>,except_t> resolve( const string_t& cmd ) const noexcept { 
    except_t err; do {

        if( is_used() )
          { return except_t( "SQL Error: client is already used" ); }
        
        if( cmd.empty() || is_closed() || obj->fd==nullptr )
          { err = except_t( "Postgres Error: Closed" ); return; }

        PGresult *ctx = PQexec( obj->fd, cmd.data() );
        queue_t<sql_item_t> list; use();

        function_t<void,sql_item_t> cb ([=]( sql_item_t args ){ 
            list.push(args); 
        });

        if( PQresultStatus(ctx) != PGRES_TUPLES_OK  ) {
        if( PQresultStatus(ctx) != PGRES_COMMAND_OK ) { PQclear(ctx); 
            err = except_t( PQerrorMessage(obj->fd) ); break;
        }   PQclear(ctx); release(); return nullptr; }

        _postgres_::pipe pipe; 
        
        while( pipe( ctx, cb )==1 ){ /*unused*/ }
    
        /*---*/ release(); return list.data();    
    } while(0); release(); return err /*--*/ ; }

    /*─······································································─*/

    bool is_closed()    const noexcept { return obj->state & STATE::SQL_STATE_CLOSE; }
    bool is_used()      const noexcept { return obj->state & STATE::SQL_STATE_USED ; }
    void close()        const noexcept { /*--*/ obj->state = STATE::SQL_STATE_CLOSE; }
    bool is_available() const noexcept { return !is_closed(); }

    /*─······································································─*/

    void free() const noexcept {
        if( obj->fd == nullptr ){ return; }
        if( is_closed() ) /*-*/ { return; } close();
    }

};}

/*────────────────────────────────────────────────────────────────────────────*/

#endif
