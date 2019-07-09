defmodule ExtendedApi.Worker.FindTransactions.Approvees.Helper do

  @moduledoc """
    This module hold all the required helper functions
    Which is gonna be used by FindTransactions.Approvees worker
  """

  alias ExtendedApi.Worker.FindTransactions.{Approvees, Approvees.EdgeFn}
  alias Core.DataModel.{Keyspace.Tangle, Table.Edge}
  import OverDB.Builder.Query

  # these types check guards
  defguardp is_approve(approve) when is_binary(approve)

  @initial_acc %{hashes: [], queries_states: []}
  @max_bigint 9223372036854775807 # will be used to generate random qf.
  @edge_cql "SELECT v2,ts,ex,ix,lx FROM tangle.edge WHERE v1 = ? AND lb = 50"
  @last_tx_bundle_cql "SELECT b FROM tangle.bundle WHERE bh = ? AND lb = 30 AND ts = ? AND ix = ? AND id = ?"
  @bundle_cql "SELECT e FROM tangle.bundle WHERE bh = ? AND lb = 30 AND ix < ? AND id = ? ALLOW FILTERING"
  # Start of Helper functions for edge table queries ###########################

  @doc """
    This function takes Approvees as list and Worker State then
    return tuple
      {:ok, state} :
        state is updated map which include all the new queries_states,
        and initial hashes
      {:error, term} : error occurs either because of invalid
        approvee structure/type or dead shard stage in the query engine.
  """
  @spec queries(list,map, list, integer) :: {:ok, map} | {:error, term}
  def queries(approvees, state, queries_states_list \\ [], ref \\ 0)
  def queries(approvees, state, queries_states_list, ref) do
    _queries(approvees, state, queries_states_list, ref)
  end

  @spec _queries(list,map, list, integer) :: tuple
  defp _queries([approve | rest], state, queries_states_list, ref) when is_approve(approve) do
    {ok?, _, q_s} = edge_query(approve, ref)
    _queries(ok?,rest,state, queries_states_list, ref, q_s)
  end

  @spec _queries(list,map, list, integer) :: tuple
  defp _queries([],%{from: from}, queries_states_list, ref) do
    # ref indicates the total number of queries.
    # queries_states_list is a list which hold all the
    # generated queries_states that needed to decode future responses.
    state =
      Enum.into(
        queries_states_list,
        %{ref: ref, hashes: [], from: from}
        )
    # return state to worker
    {:ok, state}
  end

  @spec _queries(list,map, list, integer) :: tuple
  defp _queries(_,_, _, _) do
    {:error, :invalid}
  end

  @spec _queries(atom, list, map, list, integer, map) :: tuple
  defp _queries(:ok,rest,state, queries_states_list, ref, q_s) do
    # :ok indicates ref => q_s has been received by the shard's stage.
    # therefore we should put that in queries_states_list and increase the ref.
    # now loop through the rest with updated ref/queries_states_list.
    _queries(rest,state, [{ref, q_s} | queries_states_list], ref+1)
  end

  @spec _queries(term, list, map, list, integer, map) :: tuple
  defp _queries(ok?,_,_, _, _, _) do
    {:error, ok?}
  end

  @spec edge_query(binary, integer, nil | map) :: tuple
  def edge_query(approve,ref, opts \\ nil) do
    {Tangle, Edge}
    # v2 hold bundle_hash,ts is timestamp, ex hold attachmentID which is the headHash(index=0)
    # ix hold 0(trunk) or 1(branch), lx is last_index.
    |> select([:v2,:ts,:ex,:ix,:lx]) |> type(:stream)
    |> assign(approve: approve, acc: @initial_acc)
    |> cql(@edge_cql)
    |> values([{:blob, approve}])
    |> opts(opts || %{function: {EdgeFn, :bundle_queries}})
    |> pk([v1: approve]) |> prepare?(true) |> reference({:edge, ref})
    |> Approvees.query()
  end

  @spec bundle_query(integer, integer, binary,binary,integer,integer,map) :: tuple
  def bundle_query(ix,lx,bh,id,ts,ref \\ :rand.uniform(@max_bigint), opts \\ nil)
  @doc """
    This function generates bundle_query for tip0(when approve is trunk = 0)
    it uses the follow cql:
    "SELECT b FROM tangle.bundle WHERE bh = ? AND lb = 30 AND ts = ? AND ix = ? AND id = ?"
    b column hold the hash of transaction(currentIndex=lastIndex),
    with attachmentID id, and current_index less_than(!=) last_index.
  """
  #
  def bundle_query(1,lx,bh,id,ts,ref,opts) do
    {Tangle, Bundle}
    |> select([:b]) |> type(:stream)
    |> assign(bh: bh, ix: 1, lx: lx, id: id, ts: ts)
    |> cql(@last_tx_bundle_cql)
    |> values([{:blob,bh},{:varint,ts},{:blob,lx},{:blob,id}])
    |> opts(opts || %{function: {BundleFn, :get_hash}})
    |> pk([bh: bh]) |> prepare?(true) |> reference({:bundle, ref})
    |> Approvees.query()
  end


  @doc """
    This function generates bundle_query for tip0(when approve is trunk = 0)
    it uses the follow cql:
    "SELECT e FROM tangle.bundle WHERE bh = ? AND lb = 30 AND ix < ? AND id = ? ALLOW FILTERING"
    e column hold the trunk, so this query will fetch all the transactions for bundle_hash bh
    with attachmentID id(head_hash), and current_index less_than(!=) last_index.
  """
  def bundle_query(0,lx,bh,id,ts,ref,opts) do
    {Tangle, Bundle}
    |> select([:e]) |> type(:stream)
    |> assign(bh: bh, ix: 0, lx: lx, id: id, ts: ts)
    |> cql(@bundle_cql)
    |> values([{:blob,bh},{:blob,lx},{:blob,id}])
    |> opts(opts || %{function: {BundleFn, :get_trunk}})
    |> pk([bh: bh]) |> prepare?(true) |> reference({:bundle, ref})
    |> Approvees.query()
  end

  # pattern matching with ix(trunk = 0) and return the right bundle_cql
  @spec bundle_cql?(integer) :: binary
  def bundle_cql?(0) do
    @bundle_cql
  end
  # pattern matching with ix(branch = 1) and return the right bundle_cql
  @spec bundle_cql?(integer) :: binary
  def bundle_cql?(1) do
    @last_tx_bundle_cql
  end
end