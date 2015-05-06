-- gui/sites.lua

local _ENV = mkmodule('gui.sites')

local gui = require('gui')
local widgets = require('gui.widgets')
local script=require('gui.script')

ARROW = string.char(26)

SiteDialog = defclass(SiteDialog, gui.FramedScreen)

SiteDialog.focus_path = 'SiteDialog'

SiteDialog.ATTRS{
    prompt = 'Type or select a site from this list',
    frame_style = gui.GREY_LINE_FRAME,
    frame_inset = 1,
    frame_title = 'Select Site',
    choices = DEFAULT_NIL,
    choice_str = DEFAULT_NIL,
    none_caption = 'none',
    show_none = false,
    show_filter = false,
    site_filter = DEFAULT_NIL,
    on_select = DEFAULT_NIL,
    on_cancel = DEFAULT_NIL,
    on_close = DEFAULT_NIL,
}

function SiteDialog:init(info)
    self:addviews{
        widgets.Label{
            text = {
                self.prompt, '\n\n',
                'Category: ', { text = self:cb_getfield('context_str'), pen = COLOR_CYAN }
            },
            text_pen = COLOR_WHITE,
            frame = { l = 0, t = 0 },
        },
        widgets.Label{
            view_id = 'back',
            visible = false,
            text = { { key = 'LEAVESCREEN', text = ': Back' } },
            frame = { r = 0, b = 0 },
            auto_width = true,
        },
        widgets.FilteredList{
            view_id = 'list',
            not_found_label = 'No matching sites',
            frame = { l = 0, r = 0, t = 4, b = 2 },
            icon_width = 2,
            on_submit = self:callback('onSubmitItem'),
        },
        widgets.Label{
            text = { {
                key = 'SELECT', text = ': Select',
                disabled = function() return not self.subviews.list:canSubmit() end
            } },
            frame = { l = 0, b = 0 },
        }
    }
    self:initBuiltinMode()
end

function SiteDialog:getWantedFrameSize(rect)
    return math.max(self.frame_width or 50, #self.prompt), math.min(28, rect.height-8)
end

function SiteDialog:onDestroy()
    if self.on_close then
        self.on_close()
    end
end

function SiteDialog:initBuiltinMode()
    local sublist = {}
    if self.show_none then
        table.insert(sublist, { text = self.none_caption, site_type = -1, site_index = -1 })
    end
    if self.show_filter then
      table.insert(sublist, { icon = ARROW, text = 'Filters', cb = self:callback('initFilterMode') })
    end
    for i,v in pairs(self.choices) do
        self:addSite(sublist, v, i)
    end
    self:pushContext('All sites', sublist)
end

function SiteDialog:initFilterMode()
    local sublist = {}
    local addFilter = function(text, filter)
      table.insert(sublist, {icon = ARROW, text = text, cb = self:callback('initSubFilterMode'), filter = filter })
    end
    addFilter('Undiscovered', function(s) return s.flags.Undiscovered end)
    addFilter('Discovered', function(s) return not s.flags.Undiscovered end)
    local i=0
    while true do
      local name = df.world_site_type[i]
      local site_type = i
      if name == nil then break end
      addFilter(name, function(s) return s.type == site_type end)
      i = i + 1
    end
    self:pushContext('Site Types', sublist)
end

function SiteDialog:initSubFilterMode(idx, item)
    local sublist = {}
    local text = item.text
    local filter = item.filter
    if filter == nil then 
      filter = function(x) return true end 
    end
    for i,v in pairs(self.choices) do
      if filter(v) then
        self:addSite(sublist, v, i)
      end
    end
    self:pushContext('Filter: '..text, sublist)
end

function SiteDialog:addSite(choices, site, idx)
    -- Check the filter
    if self.site_filter and not self.site_filter(site, parent, idx) then
        return
    end

    -- Find the site name
    local state = 0
    local name = nil
    if choice_str ~= nil then
      name = choice_str(site)
    else
      name = dfhack.TranslateName(site.name, true, false)
    end
      
    table.insert(choices, {
        text = name,
        search_key = key,
        site = site,
        site_type = typ, site_index = idx
    })
end

function SiteDialog:pushContext(name, choices)
    if not self.back_stack then
        self.back_stack = {}
        self.subviews.back.visible = false
    else
        table.insert(self.back_stack, {
            context_str = self.context_str,
            all_choices = self.subviews.list:getChoices(),
            edit_text = self.subviews.list:getFilter(),
            selected = self.subviews.list:getSelected(),
        })
        self.subviews.back.visible = true
    end

    self.context_str = name
    self.subviews.list:setChoices(choices, 1)
end

function SiteDialog:onGoBack()
    local save = table.remove(self.back_stack)
    self.subviews.back.visible = (#self.back_stack > 0)

    self.context_str = save.context_str
    self.subviews.list:setChoices(save.all_choices)
    self.subviews.list:setFilter(save.edit_text, save.selected)
end

function SiteDialog:submitSite(index, site)
    self:dismiss()

    if self.on_select then
        self.on_select(index, site)
    end
end

function SiteDialog:onSubmitItem(idx, item)
    if item.cb then
        item:cb(item)
    else
        self:submitSite(idx, item.site)
    end
end

function SiteDialog:onInput(keys)
    if keys.LEAVESCREEN or keys.LEAVESCREEN_ALL then
        if self.subviews.back.visible and not keys.LEAVESCREEN_ALL then
            self:onGoBack()
        else
            self:dismiss()
            if self.on_cancel then
                self.on_cancel()
            end
        end
    else
        self:inputToSubviews(keys)
    end
end

function showSitePrompt(title, prompt, choices, on_select, on_cancel, site_filter)
  if on_select == nil then on_select = script.mkresume(true) end
  if on_cancel == nil then on_cancel = script.mkresume(false) end
  -- if site_filter == nil then site_filter = script.qresume(nil) end
 
  SiteDialog{
    frame_title = title,
    prompt = prompt,
    choices = choices,
    show_filter = true,
    site_filter = site_filter,
    on_select = on_select,
    on_cancel = on_cancel,
  }:show()
  return script.wait()
end

return _ENV