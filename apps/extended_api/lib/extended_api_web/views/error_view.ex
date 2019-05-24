defmodule ExtendedApiWeb.ErrorView do

  use ExtendedApiWeb, :view

  @doc """
    This is a render view function which handle the
    Api call has not specified command or specified
    invalid/unsupported command.
  """
  @spec render(binary, map) :: Plug.Conn.t
  def render("command.json", _) do
    %{error: "'command' parameter has not been specified"}
  end

  @doc """
    This is a render view function which handle
    invalid getTrytes API call.
  """
  @spec render(binary, map) :: Plug.Conn.t
  def render("getTrytes.json", _) do
    %{error: "parameter: hashes is not provided"}
  end

  @doc """
    This is a render view function which handle the
    Api call has not specified command or specified
    invalid/unsupported command.
  """
  @spec render(binary, map) :: Plug.Conn.t
  def render("invalidType.json", _) do
    %{error: "Invaild Type"}
  end

  @spec render(binary, map) :: Plug.Conn.t
  def render("something.json", _) do
    %{error: "something went wrong"}
  end

  @spec render(binary, map) :: Plug.Conn.t
  def render("timeout.json", _) do
    %{error: "timeout"}
  end
end
