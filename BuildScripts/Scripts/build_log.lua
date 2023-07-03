function LogInfo(format, ...)
    local args = {...}
    print(string.format(format, table.unpack(args)))
end

function LogHighlight(format, ...)
    local args = {...}
    term.pushColor(term.green)
    print(string.format(format, table.unpack(args)))
    term.popColor()
end

function LogHighlightWarning(format, ...)
    local args = {...}
    term.pushColor(term.yellow)
    print(string.format(format, table.unpack(args)))
    term.popColor()
end

function LogWarning(format, ...)
    local args = {...}
    term.pushColor(term.yellow)
    print("Warning: " .. string.format(format, table.unpack(args)))
    term.popColor()
end

function LogError(format, ...)
    local args = {...}
    term.pushColor(term.red)
    print("Error: " .. string.format(format, table.unpack(args)))
    term.popColor()
end