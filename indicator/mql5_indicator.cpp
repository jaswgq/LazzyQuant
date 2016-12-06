#include "mql5_indicator.h"
#include "bar.h"

static double price_open(double open, double high, double low, double close)
{
    return open;
}

static double price_high(double open, double high, double low, double close)
{
    return high;
}

static double price_low(double open, double high, double low, double close)
{
    return low;
}

static double price_close(double open, double high, double low, double close)
{
    return close;
}

static double price_median(double open, double high, double low, double close)
{
    return (high + low) / 2.0;
}

static double price_typical(double open, double high, double low, double close)
{
    return (high + low + close) / 3.0;
}

static double price_weighted(double open, double high, double low, double close)
{
    return (high + low + close + close) / 4.0;
}

MQL5Indicator::MQL5Indicator(int indicator_buffers) :
    rates_total(0),
    prev_calculated(0),
    indicator_buffers(indicator_buffers)
{
    remaped = false;
}

MQL5Indicator::~MQL5Indicator()
{
    if (remaped) {
        delete time;
        delete open;
        delete high;
        delete low;
        delete close;
    }
}

void MQL5Indicator::setBarList(QList<Bar> *list, Bar &last)
{
    AbstractIndicator::setBarList(list, last);
    time = new RemapListMember<Bar, uint>(list, &Bar::time, last);
    open = new RemapListMember<Bar, double>(list, &Bar::open, last);
    high = new RemapListMember<Bar, double>(list, &Bar::high, last);
    low = new RemapListMember<Bar, double>(list, &Bar::low, last);
    close = new RemapListMember<Bar, double>(list, &Bar::close, last);

    // tick_volume, volume and spread are not implemented yet, don't use them in indicators
    tick_volume = new RemapListMember<Bar, long>(list, &Bar::tick_volume, last);
    volume = nullptr;
    spread = nullptr;

    remaped = true;
}

void MQL5Indicator::update()
{
    preCalculate();
    prev_calculated = OnCalculate(rates_total,
                                  prev_calculated,
                                  *time,
                                  *open,
                                  *high,
                                  *low,
                                  *close,
                                  *tick_volume,
                                  *volume,
                                  *spread
                                  );
}

void MQL5Indicator::SetIndexBuffer(int index, IndicatorBuffer<double> & buffer, ENUM_INDEXBUFFER_TYPE)
{
    indicator_buffers[index] = &buffer;
}

void MQL5Indicator::preCalculate()
{
    // TODO calculate m_rates_total
    foreach (IndicatorBuffer<double> *buffer, indicator_buffers) {
        buffer->resize(rates_total);
    }
}

MQL5IndicatorOnSingleDataBuffer::MQL5IndicatorOnSingleDataBuffer(int indicator_buffers, ENUM_APPLIED_PRICE applied_price) :
    MQL5Indicator(indicator_buffers),
    applied_price(applied_price)
{
    switch (applied_price) {
    case PRICE_OPEN:
        simplify_func = price_open;
        break;
    case PRICE_HIGH:
        simplify_func = price_high;
        break;
    case PRICE_LOW:
        simplify_func = price_low;
        break;
    case PRICE_CLOSE:
        simplify_func = price_close;
        break;
    case PRICE_MEDIAN:
        simplify_func = price_median;
        break;
    case PRICE_TYPICAL:
        simplify_func = price_typical;
        break;
    case PRICE_WEIGHTED:
        simplify_func = price_weighted;
        break;
    default:
        simplify_func = price_close;
        break;
    }
}

void MQL5IndicatorOnSingleDataBuffer::preCalculate()
{
    MQL5Indicator::preCalculate();
    applied_price_buffer.resize(rates_total);
}

int MQL5IndicatorOnSingleDataBuffer::OnCalculate(const int rates_total,                     // size of input time series
                                                 const int prev_calculated,                 // bars handled in previous call
                                                 const _TimeSeries<uint>& time,             // Time
                                                 const _TimeSeries<double>& open,           // Open
                                                 const _TimeSeries<double>& high,           // High
                                                 const _TimeSeries<double>& low,            // Low
                                                 const _TimeSeries<double>& close,          // Close
                                                 const _TimeSeries<long>& tick_volume,      // Tick Volume
                                                 const _TimeSeries<long>& volume,           // Real Volume
                                                 const _TimeSeries<int>& spread             // Spread
                                                 )
{
    int limit;
    if (prev_calculated == 0) {
        limit = 0;
    } else {
        limit = prev_calculated - 1;
    }
    for (int i = limit; i < rates_total; i++) {
        applied_price_buffer[i] = simplify_func(open[i], high[i], low[i], close[i]);
    }
    return OnCalculate(rates_total, prev_calculated, 0, applied_price_buffer);
}
