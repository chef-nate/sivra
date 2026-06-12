local function breakable_code(element)
  local document = pandoc.Pandoc({pandoc.Plain({element})})
  local rendered = pandoc.write(document, "latex"):gsub("%s+$", "")
  local prefix = "\\texttt{"

  if rendered:sub(1, #prefix) == prefix and rendered:sub(-1) == "}" then
    local contents = rendered:sub(#prefix + 1, -2)
    return pandoc.RawInline(
      "latex",
      "\\texttt{\\seqsplit{" .. contents .. "}}"
    )
  end

  return nil
end

local function wrap_inline_code(element)
  if not FORMAT:match("latex") then
    return nil
  end

  return element:walk({Code = breakable_code})
end

function Para(element)
  return wrap_inline_code(element)
end

function Plain(element)
  return wrap_inline_code(element)
end
