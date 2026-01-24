function patch(data, path)
  if type(data) ~= "table" then
    return
  end

  local okContexts, contexts = pcall(function()
    return data.genericScriptContexts
  end)

  if (not okContexts) or type(contexts) ~= "table" then
    contexts = {}
    data.genericScriptContexts = contexts
  end

  local okIb, ib = pcall(function()
    return contexts.ib_itemblacklist
  end)

  if (not okIb) or ib == nil then
    contexts.ib_itemblacklist = "/scripts/opensb/util/empty.lua"
    return data
  end
end
