classname = "button_lua"

function getPinLayout ()
	-- number, [input | output | inout], row on device, index on device, name
	return {1, "output", 0, 0, "output"}
end

active_low = true

function getConfig()
	return {"active_low", active_low}
end

function setConfig(conf)
	active_low = conf["active_low"]
end

is_active = false

function getPin(number)
	if number == 1 then
		if is_active and active_low then
			return "LOW"
		else if is_active then
			return "HIGH"
		end
		end
	end
	return "UNSET"
end

function getGraphBufferLayout()
	return {2, 2, "rgba"}
end

function drawArea()
	for x = 0, buffer_width-1 do
		for y = 0, buffer_height-1 do
			if is_active then
				setGraphbuffer(x, y, graphbuf.Pixel(255,0,0,128))
			else
				setGraphbuffer(x, y, graphbuf.Pixel(0,0,0,128))
			end
		end
	end
end

function initializeGraphBuffer()
	drawArea()
end

function onClick(active)
	is_active = active
	drawArea()
end

function onKeypress(key, active)
	onClick(active)
end
