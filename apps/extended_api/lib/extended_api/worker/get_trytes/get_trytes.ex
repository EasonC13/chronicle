defmodule ExtendedApi.Worker.GetTrytes do


  # NOTE:  this is WORK in Progress. (not ready yet)

  use GenServer
  use OverDB.Worker,
    executor: Core.Executor

  alias ExtendedApi.Worker.GetTrytes.Helper

  @doc """
    This function handing starting GetTrytes worker.
    it takes hashes list as an argument.
  """
  @spec start_link() :: tuple
  def start_link() do
    GenServer.start_link(__MODULE__, %{})
  end

  @doc """
    This function initate GetTrytes worker
  """
  @spec init(map) :: tuple
  def init(state) do
    {:ok, state}
  end

  @doc """
    This function handle the call from the processor which
    started this processor, it stores a from reference
    and block on the caller
  """
  @spec handle_call(atom, tuple, map) :: tuple
  def handle_call({:get_trytes, hashes}, from, state) do
    send(self(), {:hashes, hashes})
    state_map = Map.put(state, :from, from)
    {:noreply, state_map}
  end

  @doc """
    by looping through the hashes and create edge query
    for each hash, and it will break the api call
    if any interrupt occur.
  """
  @spec handle_info(tuple, map) :: tuple
  def handle_info({:hashes, hashes}, %{from: from} = state_map) do
    # create and send queries to scyllaDB
    # we start with queries that belong to edge table
    # to fetch the required information that enable us
    # later to fetch the trytes from bundle table.
    case Helper.edge_queries(hashes, state_map) do
      {:ok, state} ->
        {:noreply, state}
      {:error, reason} ->
        # we break, thus we should return error to client
        # before breaking.
        reply(from, {:error, reason})
        {:stop, :normal, hashes}
    end
  end

  @doc """
    This function handle full response from edge table.
    a full response might be an error(read-timeout, unprepared),
    or most important a result.
    Result after decode_full function:
    - Compute%{result: []}
      Empty list indicates that there is no row for
      txhash in trytes_list at index qf.

    - Compute%{result: tuple}
      tuple indicates that there was a row for
      txhash in trytes_list at index qf, and
      also we have successfully created and send
      a bundle_query.

    Error after decode_full function:
    - %Error{reason: :unprepared}
      This indicates that scylla had evicted the prepared statement,
      thus a retry is required.

    - other %Error{} reasons are possible for instance, :read_timeout.
      we will treat those error with breaking the API call.

  """
  @spec handle_cast(tuple, tuple) :: tuple
  def handle_cast({:full, {:edge, qf}, buffer}, {{ref, trytes_list}, state_map} = state) do
    # first we fetch the query state from the state_map using the qf key.
    query_state = Map.get(state_map, qf)
    # now we decode the buffer using the query_state.
    case Protocol.decode_full(buffer, query_state) do
      # this the tuple result.
      {%Compute{result: {_, _} = result}, _old_query_state} ->
        case result do
          {:ok, query_state} ->
            # this mean the bundle query had been sent,
            # thu we should update the state map for qf
            # with the new query_state.
            state_map = %{state_map | qf => query_state}
            {:noreply, {{ref, trytes_list}, state_map}}
          error ->
            # NOTE: we might add retry business logic.
            # we break and response.
            from = state_map[:from] # this the caller reference.
            reply(from, error)
            {:stop, :normal, state}
        end
      # empty result because the transaction in trytes_list at index(qf)
      # doesn't have a row in ScyllaDB Edge table.
      # thus no further actions should be taken for that transaction.
      {%Compute{result: []}, _old_query_state} ->
        # this reduce the ref.
        # NOTE: we reduce the ref only when the cycle for hash
        # is complete, thus empty list means no further
        # responses are expected for hash at index qf.
        ref = ref-1
        # we check if this response is the last response.
        case ref do
          0 ->
            # this indicates it's the last response
            # (or might be the first and last)
            # therefore we fulfil the API call.
            # First we fetch the from reference for the caller processor.
            from = state_map[:from] # this the caller reference.
            # Now we reply with the current trytes_list.
            reply(from, {:ok, trytes_list})
            # now we stop the worker.
            {:stop, :normal, state}
          _ ->
            # this indicates it's not the last response.
            # therefore we return the updated state.
            # we don't longer need the query_state for qf.
            state_map = Map.delete(state_map, qf)
            state = {{ref, trytes_list}, state_map}
            {:noreply, state}
        end
      # this is unprepared error handler
      %Error{reason: :unprepared} ->
        # first we use hardcoded cql statement of edge query.
        cql = "SELECT * FROM tangle.edge WHERE v1 = ? AND lb IN ?"
        # we delete the %Prepared{} struct from cache
        # this mean any future edge queries should be
        # re-prepare the %Prepared{} struct.
        FastGlobal.delete(cql)
        # now we fetch the assigned hash from the query_state
        %{hash: hash} = query_state
        # We resend the query..
        # we don't care about new generated query state neither
        # qf(query_reference) because they match the current ones.
        {ok?, _,_} = Helper.edge_query(hash, qf)
        # verfiy to proceed or break.
        ok?(ok?, state_map, state)
      %Error{reason: reason} ->
        # we break and response.
        from = state_map[:from] # this the caller reference.
        reply(from, {:error, reason})
        {:stop, :normal, state}
    end
  end

  @doc """
    This function handle full response from bundle table.
    a full response might be an error(read-timeout, unprepared),
    or most important a result.
    Result after decode_full function:
    - Compute%{result: []}
      Empty list indicates that there is no row in the bundle
      table for transaction at index(qf) in trytes_list, this
      could happen when the data consistency has not yet been
      reached as ScyllaDB is eventual consistency, or maybe the
      ZMQ feeder(future APP) is feeding the Database in
      parallel with getTrytes API request.

    - Compute%{result: map}, hash_more_pages: false
      this indicates a complete response(two rows
      address's row and transaction's row.) is ready
      where the map should hold all the required
      fields to build the transaction trytes.

      # NOTE: those assumptions has not yet been tested.
    - Compute%{result: map}, hash_more_pages: true, paging_state: ..
      this indicates the response contains only
      address's row, so we must send new bundle query request with
      the paging_state to recv the transaction's row..

    Error after decode_full function:
    - %Error{reason: :unprepared}
      This indicates that scylla had evicted the prepared statement,
      thus a retry is required.

    - other %Error{} reasons are possible for instance, :read_timeout.
      we will treat those errors with breaking the API call.

  """
  @spec handle_cast(tuple, tuple) :: tuple
  def handle_cast({:full, {:bundle, qf}, buffer}, {{ref, trytes_list}, state_map} = state) do
    # first we fetch the query state from the state_map using the qf key.
    query_state = Map.get(state_map, qf)
    # now we decode the buffer using the query_state.
    case Protocol.decode_full(buffer,query_state) do
      # this indicates the tx-object map for transaction at index(qf)
      # is ready.
      {%Compute{result: map}, %{has_more_pages: false}} ->
        # this reduce the ref.
        # NOTE: we reduce the ref only when the cycle for hash
        # is complete, thus empty list means no further
        # responses are expected for hash at index qf.
        ref = ref-1
        # we check if this response is the last response.
        case ref do
          0 ->
            # this indicates it's the last response
            # (or might be the first and last)
            # therefore we fulfil the API call.
            # First we fetch the from reference for the caller processor.
            # TODO: we should convert the map to trytes before proceeding.
            trytes_list = List.replace_at(trytes_list, qf, map)
            from = state_map[:from] # from reference.
            reply(from, {:ok, trytes_list})
            # now we stop the worker.
            {:stop, :normal, state}
          _ ->
            # this indicates it's not the last response.
            # we replace the trytes_list at index qf with
            # the map result
            # NOTE: we should convert the map to trytes before doing so
            trytes_list = List.replace_at(trytes_list, qf, map)
            # we don't longer need the query_state for qf.
            state_map = Map.delete(state_map, qf)
            # we return the updated state.
            state = {{ref, trytes_list}, state_map}
            {:noreply, state}
        end
      # this indicates the tx-object is a half_map for transaction at index(qf)
      # # NOTE: half_map only hold the address's row computing result.
      {%Compute{result: half_map}, %{has_more_pages: true, paging_state: p_state}} ->
        # create new bundle query to fetch the remaining row(transaction's row)
        # TODO:
        {:noreply, state}
      # this is unprepared error handler
      %Error{reason: :unprepared} ->
        # first we use hardcoded cql statement of bundle query.
        cql = "SELECT lb,va,a,c,d,e,f,g,h,i FROM tangle.bundle WHERE bh = ? AND lb IN ? AND ts = ? AND ix = ? AND id IN ?"
        FastGlobal.delete(cql)
        # fetch the opts from the current query_state, because it might be a
        # response for paging request.
        %{opts: opts} = query_state
        # we pass the opts as an argument to generate bundle query.
        {ok?, _, _} = Helper.bundle_query_from_opts(opts)
        # verfiy to proceed or break.
        ok?(ok?, state_map, state)
      %Error{reason: reason} ->
        # we break and response.
        from = state_map[:from] # this the caller reference.
        reply(from, {:error, reason})
        {:stop, :normal, state}
    end
  end



# TODO: WIP
####################### handling stream not ready yet...

  def handle_cast({:start, query_ref, buffer}, query_state) do
    case Protocol.decode_start(buffer, query_state) do
      {rows, query_state} ->
        {:noreply, query_state}
      %Ignore{state: query_state} ->
        # you are not suppose to do anything here
        # except returning the new query_state
        {:noreply, query_state}
    end
  end


  def handle_cast({:stream, query_ref, buffer}, query_state) do
    case Protocol.decode_stream(buffer, query_state) do
      {rows, query_state} ->
        # handle rows stream as whatever you like.
        {:noreply, query_state}
      %Ignore{state: query_state} ->
        # you are not suppose to do anything here
        # except returning the new query_state
        {:noreply, query_state}
    end
  end

  def handle_cast({:end, query_ref, buffer}, query_state) do
    case Protocol.decode_end(buffer, query_state) do
      {rows, query_state} ->
        # handle the last stream of the rows.
        {:noreply, %{}}
      response ->
        # this is %Rows{} response. which is the result
        # of select queries with prepare? false.
        {:noreply, %{}}
    end
  end

  @doc """
    Handler function which validate if query request
    has ended in the shard's socket tcp_window.
  """
  def handle_cast({:send?, _query_ref, :ok}, state) do
    {:noreply, state}
  end

  @doc """
    This match unsuccessful send requests.for simplcity
    we are droping the API call in case off any
    unsuccessful send request.
  """
  def handle_cast({:send?, _, status}, {_, %{from: from}} = state) do
    reply(from, status)
    {:stop, :normal, state}
  end

  # this check if the query has been sent
  # to the shard's stage(reporter), and its result
  # determin whether to stop GetTrytes worker or proceed.
  @spec ok?(atom, map, tuple) :: tuple
  defp ok?(ok?, state_map, state) do
    case ok? do
      :ok ->
        {:noreply, state}
      _ ->
        # we break and response.
        from = state_map[:from] # this the caller reference.
        reply(from, {:error, {:dead_shard_stage, ok?} } )
        {:stop, :normal, state}
    end
  end

  # this is used internally to reply msg to the caller.
  @spec reply(tuple, list | tuple) :: :ok
  defp reply(from, msg) do
    GenServer.reply(from, msg)
  end

  @doc """
    Await function, it will be invoked only by the processor
    which start_link this processor to fetch the result.
  """
  @spec await(pid, list, integer) :: term
  def await(pid, hashes, timeout \\ :infinity) do
    GenServer.call(pid, {:get_trytes, hashes}, timeout)
  end

  @doc """
    Execute Query function, it's public function used by helper
    module to execute queries.
  """
  @spec query(map) :: tuple
  def query(map) do
    Execute.query(map)
  end
end
