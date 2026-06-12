#include "nocterm/base/screen.h"
#include <nocterm/widgets/radiobutton.h>
#include <nocterm/base/mouse.h>

int nocterm_widget_flex_policy_set_permission(nocterm_widget_t* widget, nocterm_widget_flex_policy_permission_t permission);


NOCTERM_INTERNAL NOCTERM_WIDGET_KEY_HANDLER(nocterm_radiobutton_key_handler);

NOCTERM_INTERNAL NOCTERM_WIDGET_FOCUS_HANDLER(nocterm_radiobutton_focus_handler);

// Updates the marker cell of a radio button to reflect its current selection
// state.  Only the character changes here; the cell's existing attribute is
// preserved, because the focus handler owns the cursor/normal styling and has
// already applied the correct one (FOCUS_ENTER/LEAVE run before the activating
// key/click reaches this widget).  Re-deriving the attribute from the global
// focus pointer would be wrong during a mouse click: that pointer is only moved
// to the clicked widget AFTER its handler runs, so a just-deselected radio would
// otherwise keep the inverse cursor attribute and never clear it.
NOCTERM_INTERNAL void nocterm_radiobutton_render_marker(nocterm_radiobutton_t* radiobutton){

    nocterm_widget_t* widget = NOCTERM_WIDGET(radiobutton);

    nocterm_attribute_t attribute = (widget->buffer != NULL && widget->buffer_size > 1)
        ? widget->buffer[1].attribute
        : radiobutton->main_attribute;

    nocterm_char_t marker = radiobutton->selected ? radiobutton->select_marker : nocterm_char_from_ascii(' ');

    nocterm_widget_update(widget, 0, 1, marker, attribute);
}

nocterm_radiobutton_group_t* nocterm_radiobutton_group_new(void){

    nocterm_radiobutton_group_t* new_group = (nocterm_radiobutton_group_t*)malloc(sizeof(nocterm_radiobutton_group_t));

    if(new_group == NULL){
        return NULL;
    }

    memset(new_group, 0x0, sizeof(nocterm_radiobutton_group_t));

    if(nocterm_radiobutton_group_constructor(new_group) == NOCTERM_FAILURE){
        free(new_group);
        return NULL;
    }

    return new_group;
}

int nocterm_radiobutton_group_constructor(nocterm_radiobutton_group_t* group){

    if(group == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    group->members = NULL;
    group->members_size = 0;
    group->selected = NULL;

    return NOCTERM_SUCCESS;
}

int nocterm_radiobutton_group_destructor(nocterm_radiobutton_group_t* group){

    if(group == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    // Detach every member so that dangling group back-pointers are not left
    // behind on the radio button widgets, which the group does not own.
    for(uint64_t i = 0; i < group->members_size; i++){
        if(group->members[i] != NULL){
            group->members[i]->group = NULL;
        }
    }

    free(group->members);
    group->members = NULL;
    group->members_size = 0;
    group->selected = NULL;

    return NOCTERM_SUCCESS;
}

int nocterm_radiobutton_group_delete(nocterm_radiobutton_group_t* group){

    if(nocterm_radiobutton_group_destructor(group) == NOCTERM_FAILURE){
        return NOCTERM_FAILURE;
    }

    free(group);

    return NOCTERM_SUCCESS;
}

int nocterm_radiobutton_group_add(nocterm_radiobutton_group_t* group, nocterm_radiobutton_t* radiobutton){

    if(group == NULL || radiobutton == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    // A radio button may only belong to a single group at a time.
    if(radiobutton->group == group){
        return NOCTERM_SUCCESS;
    }

    if(radiobutton->group != NULL){
        nocterm_radiobutton_group_remove(radiobutton->group, radiobutton);
    }

    nocterm_radiobutton_t** new_members = (nocterm_radiobutton_t**)realloc(group->members, sizeof(nocterm_radiobutton_t*) * (group->members_size + 1));

    if(new_members == NULL){
        return NOCTERM_FAILURE;
    }

    group->members = new_members;
    group->members[group->members_size] = radiobutton;
    group->members_size++;

    radiobutton->group = group;

    // If the joining radio button is already selected, enforce the single
    // selection invariant by routing it through the regular selection path.
    if(radiobutton->selected){
        radiobutton->selected = false;
        nocterm_radiobutton_select(radiobutton);
    }

    return NOCTERM_SUCCESS;
}

int nocterm_radiobutton_group_remove(nocterm_radiobutton_group_t* group, nocterm_radiobutton_t* radiobutton){

    if(group == NULL || radiobutton == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    uint64_t index = group->members_size;

    for(uint64_t i = 0; i < group->members_size; i++){
        if(group->members[i] == radiobutton){
            index = i;
            break;
        }
    }

    if(index == group->members_size){
        // Not a member of this group.
        return NOCTERM_SUCCESS;
    }

    for(uint64_t i = index; i + 1 < group->members_size; i++){
        group->members[i] = group->members[i + 1];
    }

    group->members_size--;

    if(group->members_size == 0){
        free(group->members);
        group->members = NULL;
    }else{
        nocterm_radiobutton_t** new_members = (nocterm_radiobutton_t**)realloc(group->members, sizeof(nocterm_radiobutton_t*) * group->members_size);
        if(new_members != NULL){
            group->members = new_members;
        }
    }

    if(group->selected == radiobutton){
        group->selected = NULL;
    }

    radiobutton->group = NULL;

    return NOCTERM_SUCCESS;
}

nocterm_radiobutton_t* nocterm_radiobutton_group_get_selected(nocterm_radiobutton_group_t* group){

    if(group == NULL){
        errno = EINVAL;
        return NULL;
    }

    return group->selected;
}

nocterm_radiobutton_t* nocterm_radiobutton_new(nocterm_radiobutton_group_t* group, nocterm_radiobutton_onselect_handler_t onselect_handler, bool selected, void* user_data){

    nocterm_radiobutton_t* new_radiobutton = (nocterm_radiobutton_t*)malloc(sizeof(nocterm_radiobutton_t));

    if(new_radiobutton == NULL){
        return NULL;
    }

    memset(new_radiobutton, 0x0, sizeof(nocterm_radiobutton_t));

    if(nocterm_radiobutton_constructor(new_radiobutton, group, onselect_handler, selected, user_data) == NOCTERM_FAILURE){
        free(new_radiobutton);
        return NULL;
    }

    return new_radiobutton;
}

int nocterm_radiobutton_constructor(nocterm_radiobutton_t* radiobutton, nocterm_radiobutton_group_t* group, nocterm_radiobutton_onselect_handler_t onselect_handler, bool selected, void* user_data){

    if(radiobutton == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    if(nocterm_widget_constructor(NOCTERM_WIDGET(radiobutton), 1, 3, NOCTERM_WIDGET_FOCUSABLE_YES, NOCTERM_WIDGET_TYPE_REAL) == NOCTERM_FAILURE){
        return NOCTERM_FAILURE;
    }
    nocterm_widget_flex_policy_set_permission(NOCTERM_WIDGET(radiobutton), NOCTERM_WIDGET_FLEX_POLICY_PERMISSION_NONE);

    nocterm_widget_set_click_activation(NOCTERM_WIDGET(radiobutton), true);

    radiobutton->group = NULL;
    radiobutton->selected = false;
    radiobutton->onselect_handler = onselect_handler;
    radiobutton->user_data = user_data;

    nocterm_char_t left_side = {
        .bytes = {'('},
        .bytes_size = 1,
        .is_utf8 = false
    };
    nocterm_char_t right_side = {
        .bytes = {')'},
        .bytes_size = 1,
        .is_utf8 = false
    };

    nocterm_char_t select_character = {
        .bytes = {'*'},
        .bytes_size = 1,
        .is_utf8 = false
    };

    radiobutton->main_attribute = NOCTERM_ATTRIBUTE_EMPTY;
    radiobutton->cursor_attribute = radiobutton->main_attribute;
    radiobutton->cursor_attribute.inverse = true;
    radiobutton->left_side = left_side;
    radiobutton->right_side = right_side;
    radiobutton->select_marker = select_character;

    nocterm_widget_update(NOCTERM_WIDGET(radiobutton), 0, 0, left_side, radiobutton->main_attribute);
    nocterm_widget_update(NOCTERM_WIDGET(radiobutton), 0, 1, nocterm_char_from_ascii(' '), radiobutton->main_attribute);
    nocterm_widget_update(NOCTERM_WIDGET(radiobutton), 0, 2, right_side, radiobutton->main_attribute);

    nocterm_widget_set_key_handler(NOCTERM_WIDGET(radiobutton), nocterm_radiobutton_key_handler);

    nocterm_widget_set_focus_handler(NOCTERM_WIDGET(radiobutton), nocterm_radiobutton_focus_handler);

    // Joining the group first lets the selection path enforce group exclusivity.
    if(group != NULL){
        nocterm_radiobutton_group_add(group, radiobutton);
    }

    if(selected){
        nocterm_radiobutton_select(radiobutton);
    }

    return NOCTERM_SUCCESS;
}

int nocterm_radiobutton_destructor(nocterm_radiobutton_t* radiobutton){

    if(radiobutton == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    if(radiobutton->group != NULL){
        nocterm_radiobutton_group_remove(radiobutton->group, radiobutton);
    }

    if(nocterm_widget_destructor(NOCTERM_WIDGET(radiobutton)) == NOCTERM_FAILURE){
        return NOCTERM_FAILURE;
    }

    return NOCTERM_SUCCESS;
}

int nocterm_radiobutton_delete(nocterm_radiobutton_t* radiobutton){

    if(nocterm_radiobutton_destructor(radiobutton) == NOCTERM_FAILURE){
        return NOCTERM_FAILURE;
    }

    free(radiobutton);

    return NOCTERM_SUCCESS;
}

int nocterm_radiobutton_select(nocterm_radiobutton_t* radiobutton){

    if(radiobutton == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    // Selecting an already-selected radio button is a no-op; unlike a checkbox a
    // radio button cannot be toggled off by re-activating it.
    if(radiobutton->selected){
        return NOCTERM_SUCCESS;
    }

    nocterm_radiobutton_group_t* group = radiobutton->group;

    // Deselect the previously selected member of the group, if any.
    if(group != NULL && group->selected != NULL && group->selected != radiobutton){

        nocterm_radiobutton_t* previous = group->selected;

        previous->selected = false;
        nocterm_radiobutton_render_marker(previous);

        if(previous->onselect_handler){
            previous->onselect_handler(NOCTERM_WIDGET(previous), NOCTERM_RADIOBUTTON_ACTION_DESELECT, previous->user_data);
        }
    }

    radiobutton->selected = true;
    nocterm_radiobutton_render_marker(radiobutton);

    if(group != NULL){
        group->selected = radiobutton;
    }

    if(radiobutton->onselect_handler){
        radiobutton->onselect_handler(NOCTERM_WIDGET(radiobutton), NOCTERM_RADIOBUTTON_ACTION_SELECT, radiobutton->user_data);
    }

    return NOCTERM_SUCCESS;
}

bool nocterm_radiobutton_is_selected(nocterm_radiobutton_t* radiobutton){

    if(radiobutton == NULL){
        errno = EINVAL;
        return false;
    }

    return radiobutton->selected;
}

int nocterm_radiobutton_set_attribute(nocterm_radiobutton_t* radiobutton, nocterm_attribute_t attribute){

    if(radiobutton == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    pthread_mutex_lock(&NOCTERM_WIDGET(radiobutton)->lock);

    radiobutton->main_attribute = attribute;
    radiobutton->cursor_attribute = attribute;
    radiobutton->cursor_attribute.inverse = attribute.inverse ? false : true;

    if(nocterm_widget_is_focused(NOCTERM_WIDGET(radiobutton))){

        nocterm_widget_update(NOCTERM_WIDGET(radiobutton), 0, 0, NOCTERM_WIDGET(radiobutton)->buffer[0].character, radiobutton->main_attribute);
        nocterm_widget_update(NOCTERM_WIDGET(radiobutton), 0, 1, NOCTERM_WIDGET(radiobutton)->buffer[1].character, radiobutton->cursor_attribute);
        nocterm_widget_update(NOCTERM_WIDGET(radiobutton), 0, 2, NOCTERM_WIDGET(radiobutton)->buffer[2].character, radiobutton->main_attribute);

    }else{
        for(nocterm_dimension_size_t i = 0; i < 3; i++){
            nocterm_widget_update(NOCTERM_WIDGET(radiobutton), 0, i, NOCTERM_WIDGET(radiobutton)->buffer[i].character, radiobutton->main_attribute);
        }
    }

    pthread_mutex_unlock(&NOCTERM_WIDGET(radiobutton)->lock);

    return NOCTERM_SUCCESS;
}

int nocterm_radiobutton_set_marker(nocterm_radiobutton_t* radiobutton, nocterm_char_t marker){

    if(radiobutton == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    pthread_mutex_lock(&NOCTERM_WIDGET(radiobutton)->lock);

    radiobutton->select_marker = marker;

    if(radiobutton->selected){
        nocterm_radiobutton_render_marker(radiobutton);
    }

    pthread_mutex_unlock(&NOCTERM_WIDGET(radiobutton)->lock);

    return NOCTERM_SUCCESS;
}

int nocterm_radiobutton_set_sides(nocterm_radiobutton_t* radiobutton, nocterm_char_t left, nocterm_char_t right){

    if(radiobutton == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    pthread_mutex_lock(&NOCTERM_WIDGET(radiobutton)->lock);

    radiobutton->left_side = left;
    radiobutton->right_side = right;

    nocterm_widget_update(NOCTERM_WIDGET(radiobutton), 0, 0, left, radiobutton->main_attribute);
    nocterm_widget_update(NOCTERM_WIDGET(radiobutton), 0, 2, right, radiobutton->main_attribute);

    pthread_mutex_unlock(&NOCTERM_WIDGET(radiobutton)->lock);

    return NOCTERM_SUCCESS;
}

NOCTERM_WIDGET_KEY_HANDLER(nocterm_radiobutton_key_handler){

    switch(nocterm_key_translate(key)){

        // A click is delivered as a raw mouse event now that the mouse
        // controller no longer synthesizes an ENTER key on activation.
        case NOCTERM_KEY_EVENT_MOUSE:
            if(nocterm_mouse_translate(key).button != NOCTERM_MOUSE_BUTTON_LMB){
                break;
            }
        case NOCTERM_KEY_EVENT_ENTER:{
            nocterm_radiobutton_select(NOCTERM_RADIOBUTTON(self));
        }break;

        case NOCTERM_KEY_EVENT_PRINTABLE:{
            // Space or the marker character activate the radio button.
            if(key->buffer_length == 1 && (key->buffer[0] == ' ' || key->buffer[0] == '*')){
                nocterm_radiobutton_select(NOCTERM_RADIOBUTTON(self));
            }
        }break;

        default:
            break;
    }
}

NOCTERM_WIDGET_FOCUS_HANDLER(nocterm_radiobutton_focus_handler){

    // The buffer can be NULL / shrunk below the cursor cell when a flex
    // collapses the radio button width; don't dereference it on a focus
    // transition.
    if(self->buffer == NULL || self->buffer_size <= 1){
        return;
    }

    switch(focus){
        case NOCTERM_WIDGET_FOCUS_ENTER:{
            nocterm_widget_update(self, 0, 1, self->buffer[1].character, NOCTERM_RADIOBUTTON(self)->cursor_attribute);
        }break;
        case NOCTERM_WIDGET_FOCUS_LEAVE:{
            nocterm_widget_update(self, 0, 1, self->buffer[1].character, NOCTERM_RADIOBUTTON(self)->main_attribute);
        }break;
    }
}
