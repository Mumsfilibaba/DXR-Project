-- Helper function to handle colored logging with optional prefixes
local function log_with_color(color, prefix, format_str, ...)
    term.pushColor(color)
    if prefix then
        print(prefix .. string.format(format_str, ...))
    else
        print(string.format(format_str, ...))
    end
    term.popColor()
end

function log_info(format_str, ...)
    print(string.format(format_str, ...))
end

function log_highlight(format_str, ...)
    log_with_color(term.green, nil, format_str, ...)
end

function log_highlight_warning(format_str, ...)
    log_with_color(term.yellow, nil, format_str, ...)
end

function log_warning(format_str, ...)
    log_with_color(term.yellow, "Warning: ", format_str, ...)
end

function log_error(format_str, ...)
    log_with_color(term.red, "Error: ", format_str, ...)
end
