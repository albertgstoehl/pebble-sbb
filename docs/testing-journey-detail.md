# Journey Detail UI Testing Guide

## Features to Test

### 1. Journey Detail Window
- Select any connection from connection list
- Verify journey detail window opens
- Check all sections are displayed
- Verify filled circle for departures, hollow for arrivals
- Verify vertical line connects sections (not after last)
- Check station names, platforms, times, delays display correctly

### 2. Text Scrolling
- Select long station name in journey detail → should scroll after 1 second
- Select connection with long text in connection list → should scroll
- Select favorite with long name in departure/destination → should scroll

### 3. Save Connection
- From connection list: long-press SELECT
- Should show "Connection saved" confirmation
- Return to main menu → verify saved connection appears

### 4. Pin Connection
- From connection list: long-press DOWN
- Should show "Connection pinned" confirmation
- Return to main menu → verify "Active Journey" section appears
- Select Active Journey → opens journey detail
- Wait for arrival time to pass → pin should auto-clear

### 5. Favorites Display
- Check departure selection → favorites show "label | name"
- Check destination selection → should match exactly
