effect = {
    id = "lerp_transform",
    name = "Lerp Transform",
    parameters = {
        { name = "Duration", type = "enum", default = "1",
          values = {"1/16", "1/8", "1/4", "1/2", "1", "2", "4", "8"} },
        { name = "Easing", type = "enum", default = "linear",
          values = {"linear", "ease_in_sine", "ease_out_sine", "ease_in_out_sine",
                    "ease_in_quad", "ease_out_quad", "ease_in_out_quad",
                    "ease_in_cubic", "ease_out_cubic", "ease_in_out_cubic",
                    "ease_in_quart", "ease_out_quart", "ease_in_out_quart",
                    "ease_in_quint", "ease_out_quint", "ease_in_out_quint"} },
        { name = "Target X", type = "number", default = "@clip.position_x" },
        { name = "Target Y", type = "number", default = "@clip.position_y" },
        { name = "Target Scale X", type = "number", default = "@clip.scale_x" },
        { name = "Target Scale Y", type = "number", default = "@clip.scale_y" },
        { name = "Target Rotation", type = "number", default = "@clip.rotation" },
        { name = "Loop", type = "bool", default = "false" },
        { name = "Ping Pong", type = "bool", default = "false" }
    }
}

local function parse_duration(str)
    if str == "1/16" then return 0.25
    elseif str == "1/8" then return 0.5
    elseif str == "1/4" then return 1.0
    elseif str == "1/2" then return 2.0
    else return tonumber(str) or 1.0
    end
end

local function ease(t, easing)
    local pi = 3.14159265359

    if easing == "ease_in_sine" then
        return 1 - math.cos((t * pi) / 2)
    elseif easing == "ease_out_sine" then
        return math.sin((t * pi) / 2)
    elseif easing == "ease_in_out_sine" then
        return -(math.cos(pi * t) - 1) / 2

    elseif easing == "ease_in_quad" then
        return t * t
    elseif easing == "ease_out_quad" then
        return 1 - (1 - t) * (1 - t)
    elseif easing == "ease_in_out_quad" then
        if t < 0.5 then
            return 2 * t * t
        else
            return 1 - math.pow(-2 * t + 2, 2) / 2
        end

    elseif easing == "ease_in_cubic" then
        return t * t * t
    elseif easing == "ease_out_cubic" then
        return 1 - math.pow(1 - t, 3)
    elseif easing == "ease_in_out_cubic" then
        if t < 0.5 then
            return 4 * t * t * t
        else
            return 1 - math.pow(-2 * t + 2, 3) / 2
        end

    elseif easing == "ease_in_quart" then
        return t * t * t * t
    elseif easing == "ease_out_quart" then
        return 1 - math.pow(1 - t, 4)
    elseif easing == "ease_in_out_quart" then
        if t < 0.5 then
            return 8 * t * t * t * t
        else
            return 1 - math.pow(-2 * t + 2, 4) / 2
        end

    elseif easing == "ease_in_quint" then
        return t * t * t * t * t
    elseif easing == "ease_out_quint" then
        return 1 - math.pow(1 - t, 5)
    elseif easing == "ease_in_out_quint" then
        if t < 0.5 then
            return 16 * t * t * t * t * t
        else
            return 1 - math.pow(-2 * t + 2, 5) / 2
        end

    else
        return t
    end
end

function evaluate(context, params)
    local duration_beats = parse_duration(params["Duration"])
    if duration_beats <= 0 then duration_beats = 1.0 end

    local easing = params["Easing"] or "linear"

    local target_x = tonumber(params["Target X"]) or 0
    local target_y = tonumber(params["Target Y"]) or 0
    local target_scale_x = tonumber(params["Target Scale X"]) or 1
    local target_scale_y = tonumber(params["Target Scale Y"]) or 1
    local target_rotation = tonumber(params["Target Rotation"]) or 0

    local loop = params["Loop"] == "true"
    local ping_pong = params["Ping Pong"] == "true"

    local start_x = context.base.position_x
    local start_y = context.base.position_y
    local start_scale_x = context.base.scale_x
    local start_scale_y = context.base.scale_y
    local start_rotation = context.base.rotation

    local t = context.clip_local_beats / duration_beats

    if loop then
        if ping_pong then
            local cycle = math.floor(t)
            t = t - cycle
            if cycle % 2 == 1 then
                t = 1 - t
            end
        else
            t = t - math.floor(t)
        end
    else
        t = math.min(t, 1.0)
    end

    t = ease(t, easing)

    local position_x = start_x + (target_x - start_x) * t
    local position_y = start_y + (target_y - start_y) * t
    local scale_x = start_scale_x + (target_scale_x - start_scale_x) * t
    local scale_y = start_scale_y + (target_scale_y - start_scale_y) * t
    local rotation = start_rotation + (target_rotation - start_rotation) * t

    local acc_sign_x = context.clip.scale_x >= 0 and 1 or -1
    local acc_sign_y = context.clip.scale_y >= 0 and 1 or -1

    return {
        position_x = position_x,
        position_y = position_y,
        scale_x = math.abs(scale_x) * acc_sign_x,
        scale_y = math.abs(scale_y) * acc_sign_y,
        rotation = rotation
    }
end
