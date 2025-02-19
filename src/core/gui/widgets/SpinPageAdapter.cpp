#include "SpinPageAdapter.h"

#include <cstdint>  // for uint64_t

#include <glib-object.h>  // for g_signal_handler_disconnect, G_CALLBACK

#include "util/Assert.h"  // for xoj_assert
#include "util/glib_casts.h"  // for wrap_for_once_v

SpinPageAdapter::SpinPageAdapter() = default;

SpinPageAdapter::~SpinPageAdapter() {
    if (this->widget && this->pageNrSpinChangedHandlerId) {
        // In case the widget outlives *this
        g_signal_handler_disconnect(this->widget.get(), this->pageNrSpinChangedHandlerId);
    }
}

auto SpinPageAdapter::pageNrSpinChangedTimerCallback(SpinPageAdapter* adapter) -> bool {
    adapter->lastTimeoutId = 0;
    adapter->page = static_cast<size_t>(gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(adapter->widget.get())));

    adapter->firePageChanged();
    return false;
}

void SpinPageAdapter::pageNrSpinChangedCallback(GtkSpinButton* spinbutton, SpinPageAdapter* adapter) {
    // Nothing changed.
    if (gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spinbutton)) == static_cast<uint64_t>(adapter->page)) {
        return;
    }

    if (adapter->lastTimeoutId) {
        g_source_remove(adapter->lastTimeoutId);
    }

    // Give the spin button some time to release, if we don't do he will send new events...
    adapter->lastTimeoutId = g_timeout_add(100, xoj::util::wrap_for_once_v<pageNrSpinChangedTimerCallback>, adapter);
}

bool SpinPageAdapter::hasWidget() { return this->widget != nullptr; }

void SpinPageAdapter::setWidget(GtkWidget* widget) {
    if (this->widget && this->pageNrSpinChangedHandlerId) {
        g_signal_handler_disconnect(this->widget.get(), this->pageNrSpinChangedHandlerId);
    }
    xoj_assert(widget);
    this->widget.reset(widget, xoj::util::refsink);
    this->pageNrSpinChangedHandlerId =
            g_signal_connect(widget, "value-changed", G_CALLBACK(pageNrSpinChangedCallback), this);
    this->lastTimeoutId = 0;

    gtk_spin_button_set_range(GTK_SPIN_BUTTON(widget), static_cast<double>(this->min), static_cast<double>(this->max));
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(widget), static_cast<double>(this->page));
}

auto SpinPageAdapter::getPage() const -> size_t { return this->page; }

void SpinPageAdapter::setPage(size_t page) {
    this->page = page;
    if (this->widget) {
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(this->widget.get()), static_cast<double>(page));
    }
}

void SpinPageAdapter::setMinMaxPage(size_t min, size_t max) {
    this->min = min;
    this->max = max;
    if (this->widget) {
        gtk_spin_button_set_range(GTK_SPIN_BUTTON(this->widget.get()), static_cast<double>(min),
                                  static_cast<double>(max));
    }
}

void SpinPageAdapter::addListener(SpinPageListener* listener) { this->listener.push_back(listener); }

void SpinPageAdapter::removeListener(SpinPageListener* listener) { this->listener.remove(listener); }

void SpinPageAdapter::firePageChanged() {
    for (SpinPageListener* listener: this->listener) { listener->pageChanged(this->page); }
}

SpinPageListener::~SpinPageListener() = default;
