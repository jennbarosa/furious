effect = {
    id = "auto_ytpmv",
    name = "Auto YTPMV",
    parameters = {
        { name = "Sync Period", type = "enum", default = "1/4",
          values = {"1/16", "1/8", "1/4", "1/2", "measure"} },
        { name = "Flip Horizontal", type = "bool", default = "false" },
        { name = "Flip Vertical", type = "bool", default = "false" }
    }
}

function evaluate(context, params)
    local period_beats = furious.period_to_beats(params["Sync Period"] or "1/4")
    local period_seconds = context.tempo.beats_to_time(period_beats)

    local position_in_period = context.clip_local_beats % period_beats
    local position_seconds = context.tempo.beats_to_time(position_in_period)

    local period_count = math.floor(context.clip_local_beats / period_beats)

    local scale_x = context.clip.scale_x
    local scale_y = context.clip.scale_y

    if params["Flip Horizontal"] == "true" and period_count % 2 == 1 then
        scale_x = -scale_x
    end

    if params["Flip Vertical"] == "true" and period_count % 2 == 1 then
        scale_y = -scale_y
    end

    return {
        use_looped_frame = true,
        loop_start_seconds = context.clip.source_start_seconds,
        loop_duration_seconds = period_seconds,
        position_in_loop_seconds = position_seconds,
        scale_x = scale_x,
        scale_y = scale_y
    }
end
