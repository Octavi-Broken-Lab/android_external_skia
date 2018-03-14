/*
 * Copyright 2017 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "SkSGTrimEffect.h"

#include "SkCanvas.h"
#include "SkStrokeRec.h"
#include "SkTrimPathEffect.h"

namespace sksg {

TrimEffect::TrimEffect(sk_sp<GeometryNode> child)
    : fChild(std::move(child)) {
    this->observeInval(fChild);
}

TrimEffect::~TrimEffect() {
    this->unobserveInval(fChild);
}

void TrimEffect::onClip(SkCanvas* canvas, bool antiAlias) const {
    canvas->clipPath(fTrimmedPath, SkClipOp::kIntersect, antiAlias);
}

void TrimEffect::onDraw(SkCanvas* canvas, const SkPaint& paint) const {
    SkASSERT(!paint.getPathEffect());

    canvas->drawPath(fTrimmedPath, paint);
}

SkPath TrimEffect::onAsPath() const {
    return fTrimmedPath;
}

SkRect TrimEffect::onRevalidate(InvalidationController* ic, const SkMatrix& ctm) {
    SkASSERT(this->hasInval());

    const auto childbounds = fChild->revalidate(ic, ctm);
    const auto path        = fChild->asPath();

    // TODO: relocate these funky semantics to a Skottie composite helper,
    //       and refactor TrimEffect as a thin SkTrimpPathEffect wrapper.
    auto start = SkTMin(fStart, fEnd) + fOffset,
         stop  = SkTMax(fStart, fEnd) + fOffset;

    sk_sp<SkPathEffect> trim;
    if (stop - start < 1) {
        start -= SkScalarFloorToScalar(start);
        stop  -= SkScalarFloorToScalar(stop);

        trim = start <= stop
            ? SkTrimPathEffect::Make(start, stop, SkTrimPathEffect::Mode::kNormal)
            : SkTrimPathEffect::Make(stop, start, SkTrimPathEffect::Mode::kInverted);
    }

    if (trim) {
        fTrimmedPath.reset();
        SkStrokeRec rec(SkStrokeRec::kHairline_InitStyle);
        SkAssertResult(trim->filterPath(&fTrimmedPath, path, &rec, &childbounds));
    } else {
        fTrimmedPath = path;
    }

    return fTrimmedPath.computeTightBounds();
}

} // namespace sksg
