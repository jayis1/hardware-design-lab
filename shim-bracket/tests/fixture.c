/* Author: jayis1. Synthetic signals, not measured hardware. */
#include "engine.h"
#include "wire.h"
#include <stdio.h>
#include <assert.h>
static Engine e;
static unsigned tick;
static void hold(unsigned raw, unsigned count)
{
    while(count--) {
        engine_sample(&e,++tick,raw);
        assert(engine_valid(&e));
        if(tick%100==0) {
            char data[768]; size_t n=wire_encode(&e,data,sizeof data);
            assert(n); fwrite(data,1,n,stdout);
        }
    }
}
int main(void)
{
    engine_init(&e); hold(0,200);
    hold(1,100); hold(5,100); hold(1,100); /* FIT 0.50 */
    hold(3,100); hold(11,100); hold(3,100); /* REFUSE 0.75 */
    hold(2,100); hold(6,100); hold(2,100); /* FIT 1.00 contradicts */
    hold(14,100); hold(2,100); /* both buttons rejected */
    hold(6,100); engine_sample(&e,tick+=2,6); hold(2,200); /* gap abort */
    assert(e.accepted==3 && e.rejected==1 && e.aborted==1 && e.conflict);
}
