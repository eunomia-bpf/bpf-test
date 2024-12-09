function(url)
    print(url)
    -- Equivalent of strcmp_my in Lua
    local function strcmp_my(a, b)
        local len_a = #a
        local len_b = #b
        local min_len = math.min(len_a, len_b)

        for i = 1, min_len do
            if a:byte(i) ~= b:byte(i) then
                return false
            end
        end

        return len_a == len_b
    end

    -- Static entries array equivalent in Lua
    local entries = {
        {"yunwei", "aaaa", "bbbb", ""},
        {"yyw", "abab", "1234", ""},
        {"mnfe", "notfound", "fe", ""},
        {"mikunot", "foundexception", "foundexception", ""}
    }

    -- Equivalent of try_match_url in Lua
    local function try_match_url(str)
        local sec1, sec2 = "", ""
        local sec1_used, sec2_used = false, false
        local last = 0

        for i = 1, #str do
            local p = str:sub(i, i)
            if p == '/' or i == #str then
                if last ~= 0 then
                    local segment = str:sub(last + 1, i - 1)
                    if not sec1_used then
                        sec1 = segment
                        sec1_used = true
                    elseif not sec2_used then
                        sec2 = segment
                        sec2_used = true
                        break
                    end
                end
                last = i
            end
        end

        if not sec1_used or not sec2_used then return 0 end

        for i = 1, #entries do
            if strcmp_my(entries[i][1], sec1) then
                for j = 2, 4 do
                    if strcmp_my(entries[i][j], sec2) then
                        return 1
                    end
                end
            end
        end

        return 0
    end
    local function extractBeforeLastSpace(str)
        local lastSpaceIndex = nil
        for i = #str, 1, -1 do
            if str:sub(i, i) == " " then
                lastSpaceIndex = i
                break
            end
        end

        if lastSpaceIndex then
            return str:sub(1, lastSpaceIndex - 1)
        else
            return str
        end
    end


    local toMatch = extractBeforeLastSpace(url)
    return try_match_url(toMatch) ~= 0
end
