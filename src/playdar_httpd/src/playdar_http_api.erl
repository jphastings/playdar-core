-module(playdar_http_api).
-export([http_req/1]).

http_req(Req) ->
    Qs = Req:parse_qs(),
    M = proplists:get_value("method", Qs),
    case M of
        "stat" ->
            R = {struct,[   
                    {"name", <<"playdar">>},
                    {"version", <<"0.1.0">>},
                    {"authenticated", true},
                    {"hostname", <<"TODO">>},
                    {"capabilities", {struct,[
                        {"local", {struct,[
                            {"plugin", <<"Local Library">>},
                            {"description", <<"Blah">>}
                        ]}}
                    ]}}
                ]},
            respond(Req, R);
        
        "resolve" ->
            Artist = proplists:get_value("artist", Qs, ""),
            Album  = proplists:get_value("album", Qs, ""),
            Track  = proplists:get_value("track", Qs, ""),
            Qid = utils:uuid_gen(),
            Q = {struct,[
                    {<<"artist">>, list_to_binary(Artist)}, 
                    {<<"album">>,  list_to_binary(Album)}, 
                    {<<"track">>,  list_to_binary(Track)}
                ]},
            _Qpid = resolver:dispatch(Q, Qid),
            R = {struct,[
                    {"qid", Qid}
                ]},
                
            respond(Req, R);
            
        "get_results" ->
            Qid = list_to_binary(proplists:get_value("qid", Qs)),
            Qpid = resolver:qid2pid(Qid),
            case Qpid of 
                undefined ->
                    Req:not_found();
                _ ->
                    Results = qry:results(Qpid),
                    %ResultsJ = [ {struct,L} || L <- Results ],
                    Q = qry:q(Qpid),
                    %QJ = {struct, [ {atom_to_list(K),V} || {K,V} <- Q ] },
                    R = {struct,[
                            {"qid", Qid},
                            {"refresh_interval", 1000},
                            {"query", Q},
                            {"results", Results}
                        ]},
                    respond(Req, R)
             end;
         
        _ ->
            Req:ok({"text/plain", io_lib:format("Method not found: ~p~n", [M])})
    end.
    
respond(Req, R) ->
    Qs = Req:parse_qs(),
    case proplists:get_value("jsonp", Qs) of
        undefined ->
            Req:ok({"text/javascript; charset=utf-8", [], mochijson2:encode(R)});
        F ->
            Req:ok({"text/javascript; charset=utf-8", [], 
                    F++"("++mochijson2:encode(R)++");\n"})
    end.
        
    
    