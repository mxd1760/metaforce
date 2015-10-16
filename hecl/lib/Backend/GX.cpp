#include "HECL/Backend/GX.hpp"
#include <map>

namespace HECL
{
namespace Backend
{

unsigned GX::addKColor(Diagnostics& diag, const SourceLocation& loc, const Color& color)
{
    for (unsigned i=0 ; i<m_kcolorCount ; ++i)
        if (m_kcolors[i] == color)
            return i;
    if (m_kcolorCount >= 4)
        diag.reportBackendErr(loc, "GX KColor overflow");
    m_kcolors[m_kcolorCount] = color;
    return m_kcolorCount++;
}

unsigned GX::addKAlpha(Diagnostics& diag, const SourceLocation& loc, float alpha)
{
    uint8_t ai = uint8_t(std::min(std::max(alpha * 255.f, 0.f), 255.f));
    for (unsigned i=0 ; i<m_kcolorCount ; ++i)
    {
        if (m_kcolors[i].color[3] == ai)
            return i;
        else if (m_kcolors[i].color[3] == 0)
        {
            m_kcolors[i].color[3] = ai;
            return i;
        }
    }
    if (m_kcolorCount >= 4)
        diag.reportBackendErr(loc, "GX KColor overflow");
    m_kcolors[m_kcolorCount] = ai;
    return m_kcolorCount++;
}

unsigned GX::addTexCoordGen(Diagnostics& diag, const SourceLocation& loc,
                            TexGenSrc src, TexMtx mtx)
{
    for (unsigned i=0 ; i<m_tcgCount ; ++i)
    {
        TexCoordGen& tcg = m_tcgs[i];
        if (tcg.m_src == src && tcg.m_mtx == mtx)
            return i;
    }
    if (m_tcgCount >= 8)
        diag.reportBackendErr(loc, "GX TexCoordGen overflow");
    GX::TexCoordGen& newTcg = m_tcgs[m_tcgCount];
    newTcg.m_src = src;
    newTcg.m_mtx = mtx;
    return m_tcgCount++;
}

GX::TEVStage& GX::addTEVStage(Diagnostics& diag, const SourceLocation& loc)
{
    if (m_tevCount >= 16)
        diag.reportBackendErr(loc, "GX TEV stage overflow");
    GX::TEVStage& newTEV = m_tevs[m_tevCount];
    newTEV.m_loc = loc;
    if (m_tevCount)
    {
        newTEV.m_prev = &m_tevs[m_tevCount-1];
        newTEV.m_prev->m_next = &newTEV;
    }
    ++m_tevCount;
    return newTEV;
}

unsigned GX::RecursiveTraceTexGen(const IR& ir, Diagnostics& diag, const IR::Instruction& inst, TexMtx mtx)
{
    if (inst.m_op != IR::OpCall)
        diag.reportBackendErr(inst.m_loc, "TexCoordGen resolution requires function");

    const std::string& tcgName = inst.m_call.m_name;
    if (!tcgName.compare("UV"))
    {
        if (inst.getChildCount() < 1)
            diag.reportBackendErr(inst.m_loc, "TexCoordGen UV(layerIdx) requires one argument");
        const IR::Instruction& idxInst = inst.getChildInst(ir, 0);
        const atVec4f& idxImm = idxInst.getImmVec();
        return addTexCoordGen(diag, inst.m_loc, TexGenSrc(TG_TEX0 + unsigned(idxImm.vec[0])), mtx);
    }
    else if (!tcgName.compare("Normal"))
        return addTexCoordGen(diag, inst.m_loc, TG_NRM, mtx);
    else if (!tcgName.compare("View"))
        return addTexCoordGen(diag, inst.m_loc, TG_POS, mtx);

    /* Otherwise treat as game-specific function */
    const IR::Instruction& tcgSrcInst = inst.getChildInst(ir, 0);
    unsigned idx = RecursiveTraceTexGen(ir, diag, tcgSrcInst, TexMtx(TEXMTX0 + m_texMtxCount * 3));
    GX::TexCoordGen& tcg = m_tcgs[idx];
    m_texMtxRefs[m_texMtxCount] = &tcg;
    ++m_texMtxCount;
    tcg.m_gameFunction = tcgName;
    tcg.m_gameArgs.clear();
    for (int i=1 ; i<inst.getChildCount() ; ++i)
    {
        const IR::Instruction& ci = inst.getChildInst(ir, i);
        tcg.m_gameArgs.push_back(ci.getImmVec());
    }
    return idx;
}

GX::TraceResult GX::RecursiveTraceColor(const IR& ir, Diagnostics& diag, const IR::Instruction& inst,
                                        bool swizzleAlpha)
{
    switch (inst.m_op)
    {
    case IR::OpCall:
    {
        const std::string& name = inst.m_call.m_name;
        if (!name.compare("Texture"))
        {
            TEVStage& newStage = addTEVStage(diag, inst.m_loc);

            if (inst.getChildCount() < 2)
                diag.reportBackendErr(inst.m_loc, "Texture(map, texgen) requires 2 arguments");

            const IR::Instruction& mapInst = inst.getChildInst(ir, 0);
            const atVec4f& mapImm = mapInst.getImmVec();
            newStage.m_texMapIdx = unsigned(mapImm.vec[0]);
            newStage.m_color[0] = swizzleAlpha ? CC_TEXA : CC_TEXC;

            const IR::Instruction& tcgInst = inst.getChildInst(ir, 1);
            newStage.m_texGenIdx = RecursiveTraceTexGen(ir, diag, tcgInst, IDENTITY);

            return TraceResult(&newStage);
        }
        else if (!name.compare("ColorReg"))
        {
            const IR::Instruction& idxInst = inst.getChildInst(ir, 0);
            unsigned idx = unsigned(idxInst.getImmVec().vec[0]);
            m_cRegMask |= 1 << idx;
            return TraceResult(TevColorArg((swizzleAlpha ? CC_A0 : CC_C0) + idx * 2));
        }
        else if (!name.compare("Lighting"))
        {
            return TraceResult(swizzleAlpha ? CC_RASA : CC_RASC);
        }
        else
            diag.reportBackendErr(inst.m_loc, "GX backend unable to interpret '%s'", name.c_str());
        break;
    }
    case IR::OpLoadImm:
    {
        const atVec4f& vec = inst.m_loadImm.m_immVec;
        if (vec.vec[0] == 0.f && vec.vec[1] == 0.f && vec.vec[2] == 0.f)
            return TraceResult(CC_ZERO);
        else if (vec.vec[0] == 1.f && vec.vec[1] == 1.f && vec.vec[2] == 1.f)
            return TraceResult(CC_ONE);
        unsigned idx = addKColor(diag, inst.m_loc, vec);
        return TraceResult(TevKColorSel(TEV_KCSEL_K0 + idx));
    }
    case IR::OpArithmetic:
    {
        ArithmeticOp op = inst.m_arithmetic.m_op;
        const IR::Instruction& aInst = inst.getChildInst(ir, 0);
        const IR::Instruction& bInst = inst.getChildInst(ir, 1);
        TraceResult aTrace;
        TraceResult bTrace;
        if (aInst.m_op != IR::OpArithmetic && bInst.m_op == IR::OpArithmetic)
        {
            bTrace = RecursiveTraceColor(ir, diag, bInst);
            aTrace = RecursiveTraceColor(ir, diag, aInst);
        }
        else
        {
            aTrace = RecursiveTraceColor(ir, diag, aInst);
            bTrace = RecursiveTraceColor(ir, diag, bInst);
        }
        if (aTrace.type == TraceResult::TraceTEVStage &&
            bTrace.type == TraceResult::TraceTEVStage &&
            getStageIdx(aTrace.tevStage) > getStageIdx(bTrace.tevStage))
        {
            TraceResult tmp = aTrace;
            aTrace = bTrace;
            bTrace = tmp;
        }

        TevKColorSel newKColor = TEV_KCSEL_1;
        if (aTrace.type == TraceResult::TraceTEVKColorSel &&
            bTrace.type == TraceResult::TraceTEVKColorSel)
            diag.reportBackendErr(inst.m_loc, "unable to handle 2 KColors in one stage");
        else if (aTrace.type == TraceResult::TraceTEVKColorSel)
        {
            newKColor = aTrace.tevKColorSel;
            aTrace.type = TraceResult::TraceTEVColorArg;
            aTrace.tevColorArg = CC_KONST;
        }
        else if (bTrace.type == TraceResult::TraceTEVKColorSel)
        {
            newKColor = bTrace.tevKColorSel;
            bTrace.type = TraceResult::TraceTEVColorArg;
            bTrace.tevColorArg = CC_KONST;
        }

        switch (op)
        {
        case ArithmeticOp::ArithmeticOpAdd:
        {
            if (aTrace.type == TraceResult::TraceTEVStage &&
                bTrace.type == TraceResult::TraceTEVStage)
            {
                TEVStage* a = aTrace.tevStage;
                TEVStage* b = bTrace.tevStage;
                if (b->m_prev != a)
                {
                    a->m_regOut = TEVLAZY;
                    b->m_color[3] = CC_LAZY;
                    b->m_lazyCInIdx = m_cRegLazy;
                    a->m_lazyOutIdx = m_cRegLazy++;
                }
                else if (b == &m_tevs[m_tevCount-1] &&
                         a->m_texMapIdx == b->m_texMapIdx && a->m_texGenIdx == b->m_texGenIdx &&
                         a->m_color[3] == CC_ZERO && b->m_color[0] != CC_ZERO)
                {
                    a->m_color[3] = b->m_color[0];
                    --m_tevCount;
                    return TraceResult(a);
                }
                else
                    b->m_color[3] = CC_CPREV;
                return TraceResult(b);
            }
            else if (aTrace.type == TraceResult::TraceTEVStage &&
                     bTrace.type == TraceResult::TraceTEVColorArg)
            {
                TEVStage* a = aTrace.tevStage;
                if (a->m_color[3] != CC_ZERO)
                    diag.reportBackendErr(inst.m_loc, "unable to modify TEV stage for add combine");
                a->m_color[3] = bTrace.tevColorArg;
                a->m_kColor = newKColor;
                return TraceResult(a);
            }
            else if (aTrace.type == TraceResult::TraceTEVColorArg &&
                     bTrace.type == TraceResult::TraceTEVStage)
            {
                TEVStage* b = bTrace.tevStage;
                if (b->m_color[3] != CC_ZERO)
                    diag.reportBackendErr(inst.m_loc, "unable to modify TEV stage for add combine");
                b->m_color[3] = aTrace.tevColorArg;
                b->m_kColor = newKColor;
                return TraceResult(b);
            }
            break;
        }
        case ArithmeticOp::ArithmeticOpSubtract:
        {
            if (aTrace.type == TraceResult::TraceTEVStage &&
                bTrace.type == TraceResult::TraceTEVStage)
            {
                TEVStage* a = aTrace.tevStage;
                TEVStage* b = bTrace.tevStage;
                if (b->m_prev != a)
                {
                    a->m_regOut = TEVLAZY;
                    b->m_color[3] = CC_LAZY;
                    b->m_lazyCInIdx = m_cRegLazy;
                    a->m_lazyOutIdx = m_cRegLazy++;
                }
                else
                    b->m_color[3] = CC_CPREV;
                b->m_op = TEV_SUB;
                return TraceResult(b);
            }
            else if (aTrace.type == TraceResult::TraceTEVStage &&
                     bTrace.type == TraceResult::TraceTEVColorArg)
            {
                TEVStage* a = aTrace.tevStage;
                if (a->m_color[3] != CC_ZERO)
                    diag.reportBackendErr(inst.m_loc, "unable to modify TEV stage for subtract combine");
                a->m_color[3] = bTrace.tevColorArg;
                a->m_kColor = newKColor;
                a->m_op = TEV_SUB;
                return TraceResult(a);
            }
            break;
        }
        case ArithmeticOp::ArithmeticOpMultiply:
        {
            if (aTrace.type == TraceResult::TraceTEVStage &&
                bTrace.type == TraceResult::TraceTEVStage)
            {
                TEVStage* a = aTrace.tevStage;
                TEVStage* b = bTrace.tevStage;
                if (b->m_color[2] != CC_ZERO)
                    diag.reportBackendErr(inst.m_loc, "unable to modify TEV stage for multiply combine");
                if (b->m_prev != a)
                {
                    a->m_regOut = TEVLAZY;
                    b->m_color[2] = CC_LAZY;
                    b->m_lazyCInIdx = m_cRegLazy;
                    a->m_lazyOutIdx = m_cRegLazy++;
                }
                else
                    b->m_color[2] = CC_CPREV;
                b->m_color[1] = b->m_color[0];
                b->m_color[0] = CC_ZERO;
                b->m_color[3] = CC_ZERO;
                return TraceResult(b);
            }
            else if (aTrace.type == TraceResult::TraceTEVColorArg &&
                     bTrace.type == TraceResult::TraceTEVColorArg)
            {
                TEVStage& stage = addTEVStage(diag, inst.m_loc);
                stage.m_color[1] = aTrace.tevColorArg;
                stage.m_color[2] = bTrace.tevColorArg;
                stage.m_kColor = newKColor;
                return TraceResult(&stage);
            }
            else if (aTrace.type == TraceResult::TraceTEVStage &&
                     bTrace.type == TraceResult::TraceTEVColorArg)
            {
                TEVStage* a = aTrace.tevStage;
                if (a->m_color[1] != CC_ZERO)
                {
                    if (a->m_regOut != TEVPREV)
                        diag.reportBackendErr(inst.m_loc, "unable to modify TEV stage for multiply combine");
                    TEVStage& stage = addTEVStage(diag, inst.m_loc);
                    stage.m_color[1] = CC_CPREV;
                    stage.m_color[2] = bTrace.tevColorArg;
                    stage.m_kColor = newKColor;
                    return TraceResult(&stage);
                }
                a->m_color[1] = a->m_color[0];
                a->m_color[0] = CC_ZERO;
                a->m_color[2] = bTrace.tevColorArg;
                a->m_kColor = newKColor;
                return TraceResult(a);
            }
            else if (aTrace.type == TraceResult::TraceTEVColorArg &&
                     bTrace.type == TraceResult::TraceTEVStage)
            {
                TEVStage* b = bTrace.tevStage;
                if (b->m_color[1] != CC_ZERO)
                {
                    if (b->m_regOut != TEVPREV)
                        diag.reportBackendErr(inst.m_loc, "unable to modify TEV stage for multiply combine");
                    TEVStage& stage = addTEVStage(diag, inst.m_loc);
                    stage.m_color[1] = aTrace.tevColorArg;
                    stage.m_color[2] = CC_CPREV;
                    stage.m_kColor = newKColor;
                    return TraceResult(&stage);
                }
                b->m_color[1] = b->m_color[0];
                b->m_color[0] = CC_ZERO;
                b->m_color[2] = bTrace.tevColorArg;
                b->m_kColor = newKColor;
                return TraceResult(b);
            }
            break;
        }
        default:
            diag.reportBackendErr(inst.m_loc, "invalid arithmetic op");
        }

        diag.reportBackendErr(inst.m_loc, "unable to convert arithmetic to TEV stage");
    }
    case IR::OpSwizzle:
    {
        if (inst.m_swizzle.m_idxs[0] == 3 && inst.m_swizzle.m_idxs[1] == -1 &&
            inst.m_swizzle.m_idxs[2] == -1 && inst.m_swizzle.m_idxs[3] == -1)
        {
            const IR::Instruction& cInst = inst.getChildInst(ir, 0);
            if (cInst.m_op != IR::OpCall)
                diag.reportBackendErr(inst.m_loc, "only functions accepted for alpha swizzle");
            return RecursiveTraceColor(ir, diag, cInst, true);
        }
        else
            diag.reportBackendErr(inst.m_loc, "only alpha extract may be performed with swizzle operation");
    }
    default:
        diag.reportBackendErr(inst.m_loc, "invalid color op");
    }

    return TraceResult();
}

GX::TraceResult GX::RecursiveTraceAlpha(const IR& ir, Diagnostics& diag, const IR::Instruction& inst)
{
    switch (inst.m_op)
    {
    case IR::OpCall:
    {
        const std::string& name = inst.m_call.m_name;
        if (!name.compare("Texture"))
        {
            if (inst.getChildCount() < 2)
                diag.reportBackendErr(inst.m_loc, "Texture(map, texgen) requires 2 arguments");

            const IR::Instruction& mapInst = inst.getChildInst(ir, 0);
            const atVec4f& mapImm = mapInst.getImmVec();
            unsigned mapIdx = unsigned(mapImm.vec[0]);

            int foundStage = -1;
            for (int i=0 ; i<int(m_tevCount) ; ++i)
            {
                TEVStage& testStage = m_tevs[i];
                if (testStage.m_texMapIdx == mapIdx && i > m_alphaTraceStage)
                {
                    foundStage = i;
                    break;
                }
            }

            if (foundStage >= 0)
            {
                m_alphaTraceStage = foundStage;
                TEVStage& stage = m_tevs[foundStage];
                stage.m_alpha[0] = CA_TEXA;
                return TraceResult(&stage);
            }

            TEVStage& newStage = addTEVStage(diag, inst.m_loc);
            newStage.m_color[3] = CC_CPREV;

            newStage.m_texMapIdx = mapIdx;
            newStage.m_alpha[0] = CA_TEXA;

            const IR::Instruction& tcgInst = inst.getChildInst(ir, 1);
            newStage.m_texGenIdx = RecursiveTraceTexGen(ir, diag, tcgInst, IDENTITY);

            return TraceResult(&newStage);
        }
        else if (!name.compare("ColorReg"))
        {
            const IR::Instruction& idxInst = inst.getChildInst(ir, 0);
            unsigned idx = unsigned(idxInst.getImmVec().vec[0]);
            m_cRegMask |= 1 << idx;
            return TraceResult(TevAlphaArg(CA_A0 + idx));
        }
        else if (!name.compare("Lighting"))
        {
            return TraceResult(CA_RASA);
        }
        else
            diag.reportBackendErr(inst.m_loc, "GX backend unable to interpret '%s'", name.c_str());
        break;
    }
    case IR::OpLoadImm:
    {
        const atVec4f& vec = inst.m_loadImm.m_immVec;
        if (vec.vec[0] == 0.f)
            return TraceResult(CA_ZERO);
        unsigned idx = addKAlpha(diag, inst.m_loc, vec.vec[0]);
        return TraceResult(TevKAlphaSel(TEV_KASEL_K0_A + idx));
    }
    case IR::OpArithmetic:
    {
        ArithmeticOp op = inst.m_arithmetic.m_op;
        const IR::Instruction& aInst = inst.getChildInst(ir, 0);
        const IR::Instruction& bInst = inst.getChildInst(ir, 1);
        TraceResult aTrace;
        TraceResult bTrace;
        if (aInst.m_op != IR::OpArithmetic && bInst.m_op == IR::OpArithmetic)
        {
            bTrace = RecursiveTraceAlpha(ir, diag, bInst);
            aTrace = RecursiveTraceAlpha(ir, diag, aInst);
        }
        else
        {
            aTrace = RecursiveTraceAlpha(ir, diag, aInst);
            bTrace = RecursiveTraceAlpha(ir, diag, bInst);
        }

        TevKAlphaSel newKAlpha = TEV_KASEL_1;
        if (aTrace.type == TraceResult::TraceTEVKAlphaSel &&
            bTrace.type == TraceResult::TraceTEVKAlphaSel)
            diag.reportBackendErr(inst.m_loc, "unable to handle 2 KAlphas in one stage");
        else if (aTrace.type == TraceResult::TraceTEVKAlphaSel)
        {
            newKAlpha = aTrace.tevKAlphaSel;
            aTrace.type = TraceResult::TraceTEVAlphaArg;
            aTrace.tevAlphaArg = CA_KONST;
        }
        else if (bTrace.type == TraceResult::TraceTEVKAlphaSel)
        {
            newKAlpha = bTrace.tevKAlphaSel;
            bTrace.type = TraceResult::TraceTEVAlphaArg;
            bTrace.tevAlphaArg = CA_KONST;
        }

        switch (op)
        {
        case ArithmeticOp::ArithmeticOpAdd:
        {
            if (aTrace.type == TraceResult::TraceTEVStage &&
                bTrace.type == TraceResult::TraceTEVStage)
            {
                TEVStage* a = aTrace.tevStage;
                TEVStage* b = bTrace.tevStage;
                if (b->m_prev != a)
                {
                    a->m_regOut = TEVLAZY;
                    b->m_alpha[3] = CA_LAZY;
                    if (a->m_lazyOutIdx != -1)
                        b->m_lazyAInIdx = a->m_lazyOutIdx;
                    else
                    {
                        b->m_lazyAInIdx = m_cRegLazy;
                        a->m_lazyOutIdx = m_cRegLazy++;
                    }
                }
                else
                    b->m_alpha[3] = CA_APREV;
                return TraceResult(b);
            }
            else if (aTrace.type == TraceResult::TraceTEVStage &&
                     bTrace.type == TraceResult::TraceTEVAlphaArg)
            {
                TEVStage* a = aTrace.tevStage;
                if (a->m_alpha[3] != CA_ZERO)
                    diag.reportBackendErr(inst.m_loc, "unable to modify TEV stage for add combine");
                a->m_alpha[3] = bTrace.tevAlphaArg;
                a->m_kAlpha = newKAlpha;
                return TraceResult(a);
            }
            else if (aTrace.type == TraceResult::TraceTEVAlphaArg &&
                     bTrace.type == TraceResult::TraceTEVStage)
            {
                TEVStage* b = bTrace.tevStage;
                if (b->m_alpha[3] != CA_ZERO)
                    diag.reportBackendErr(inst.m_loc, "unable to modify TEV stage for add combine");
                b->m_alpha[3] = aTrace.tevAlphaArg;
                b->m_kAlpha = newKAlpha;
                return TraceResult(b);
            }
            break;
        }
        case ArithmeticOp::ArithmeticOpSubtract:
        {
            if (aTrace.type == TraceResult::TraceTEVStage &&
                bTrace.type == TraceResult::TraceTEVStage)
            {
                TEVStage* a = aTrace.tevStage;
                TEVStage* b = bTrace.tevStage;
                if (b->m_op != TEV_SUB)
                    diag.reportBackendErr(inst.m_loc, "unable to integrate alpha subtraction into stage chain");
                if (b->m_prev != a)
                {
                    a->m_regOut = TEVLAZY;
                    b->m_alpha[3] = CA_LAZY;
                    if (a->m_lazyOutIdx != -1)
                        b->m_lazyAInIdx = a->m_lazyOutIdx;
                    else
                    {
                        b->m_lazyAInIdx = m_cRegLazy;
                        a->m_lazyOutIdx = m_cRegLazy++;
                    }
                }
                else
                    b->m_alpha[3] = CA_APREV;
                return TraceResult(b);
            }
            else if (aTrace.type == TraceResult::TraceTEVStage &&
                     bTrace.type == TraceResult::TraceTEVAlphaArg)
            {
                TEVStage* a = aTrace.tevStage;
                if (a->m_op != TEV_SUB)
                    diag.reportBackendErr(inst.m_loc, "unable to integrate alpha subtraction into stage chain");
                if (a->m_alpha[3] != CA_ZERO)
                    diag.reportBackendErr(inst.m_loc, "unable to modify TEV stage for add combine");
                a->m_alpha[3] = bTrace.tevAlphaArg;
                a->m_kAlpha = newKAlpha;
                return TraceResult(a);
            }
            break;
        }
        case ArithmeticOp::ArithmeticOpMultiply:
        {
            if (aTrace.type == TraceResult::TraceTEVStage &&
                bTrace.type == TraceResult::TraceTEVStage)
            {
                TEVStage* a = aTrace.tevStage;
                TEVStage* b = bTrace.tevStage;
                if (b->m_alpha[2] != CA_ZERO)
                    diag.reportBackendErr(inst.m_loc, "unable to modify TEV stage for multiply combine");
                if (b->m_prev != a)
                {
                    a->m_regOut = TEVLAZY;
                    b->m_alpha[2] = CA_LAZY;
                    b->m_lazyAInIdx = m_cRegLazy;
                    a->m_lazyOutIdx = m_cRegLazy++;
                }
                else
                    b->m_alpha[2] = CA_APREV;
                b->m_alpha[1] = b->m_alpha[0];
                b->m_alpha[0] = CA_ZERO;
                b->m_alpha[3] = CA_ZERO;
                return TraceResult(b);
            }
            else if (aTrace.type == TraceResult::TraceTEVAlphaArg &&
                     bTrace.type == TraceResult::TraceTEVAlphaArg)
            {
                TEVStage& stage = addTEVStage(diag, inst.m_loc);
                stage.m_color[3] = CC_CPREV;
                stage.m_alpha[1] = aTrace.tevAlphaArg;
                stage.m_alpha[2] = bTrace.tevAlphaArg;
                stage.m_kAlpha = newKAlpha;
                return TraceResult(&stage);
            }
            else if (aTrace.type == TraceResult::TraceTEVStage &&
                     bTrace.type == TraceResult::TraceTEVAlphaArg)
            {
                TEVStage* a = aTrace.tevStage;
                if (a->m_alpha[1] != CA_ZERO)
                {
                    if (a->m_regOut != TEVPREV)
                        diag.reportBackendErr(inst.m_loc, "unable to modify TEV stage for multiply combine");
                    TEVStage& stage = addTEVStage(diag, inst.m_loc);
                    stage.m_alpha[1] = CA_APREV;
                    stage.m_alpha[2] = bTrace.tevAlphaArg;
                    stage.m_kAlpha = newKAlpha;
                    return TraceResult(&stage);
                }
                a->m_alpha[1] = a->m_alpha[0];
                a->m_alpha[0] = CA_ZERO;
                a->m_alpha[2] = bTrace.tevAlphaArg;
                a->m_kAlpha = newKAlpha;
                return TraceResult(a);
            }
            else if (aTrace.type == TraceResult::TraceTEVAlphaArg &&
                     bTrace.type == TraceResult::TraceTEVStage)
            {
                TEVStage* b = bTrace.tevStage;
                if (b->m_alpha[1] != CA_ZERO)
                {
                    if (b->m_regOut != TEVPREV)
                        diag.reportBackendErr(inst.m_loc, "unable to modify TEV stage for multiply combine");
                    TEVStage& stage = addTEVStage(diag, inst.m_loc);
                    stage.m_alpha[1] = aTrace.tevAlphaArg;
                    stage.m_alpha[2] = CA_APREV;
                    stage.m_kAlpha = newKAlpha;
                    return TraceResult(&stage);
                }
                b->m_alpha[1] = b->m_alpha[0];
                b->m_alpha[0] = CA_ZERO;
                b->m_alpha[2] = bTrace.tevAlphaArg;
                b->m_kAlpha = newKAlpha;
                return TraceResult(b);
            }
            break;
        }
        default:
            diag.reportBackendErr(inst.m_loc, "invalid arithmetic op");
        }

        diag.reportBackendErr(inst.m_loc, "unable to convert arithmetic to TEV stage");
    }
    case IR::OpSwizzle:
    {
        if (inst.m_swizzle.m_idxs[0] == 3 && inst.m_swizzle.m_idxs[1] == -1 &&
            inst.m_swizzle.m_idxs[2] == -1 && inst.m_swizzle.m_idxs[3] == -1)
        {
            const IR::Instruction& cInst = inst.getChildInst(ir, 0);
            if (cInst.m_op != IR::OpCall)
                diag.reportBackendErr(inst.m_loc, "only functions accepted for alpha swizzle");
            return RecursiveTraceAlpha(ir, diag, cInst);
        }
        else
            diag.reportBackendErr(inst.m_loc, "only alpha extract may be performed with swizzle operation");
    }
    default:
        diag.reportBackendErr(inst.m_loc, "invalid alpha op");
    }

    return TraceResult();
}

void GX::reset(const IR& ir, Diagnostics& diag)
{
    m_tevCount = 0;
    m_tcgCount = 0;
    m_texMtxCount = 0;
    m_kcolorCount = 0;
    m_cRegMask = 0;
    m_cRegLazy = 0;
    m_alphaTraceStage = -1;

    /* Final instruction is the root call by hecl convention */
    const IR::Instruction& rootCall = ir.m_instructions.back();
    bool doAlpha = false;
    if (!rootCall.m_call.m_name.compare("HECLOpaque"))
    {
        m_blendSrc = BL_ONE;
        m_blendDst = BL_ZERO;
    }
    else if (!rootCall.m_call.m_name.compare("HECLAlpha"))
    {
        m_blendSrc = BL_SRCALPHA;
        m_blendDst = BL_INVSRCALPHA;
        doAlpha = true;
    }
    else if (!rootCall.m_call.m_name.compare("HECLAdditive"))
    {
        m_blendSrc = BL_SRCALPHA;
        m_blendDst = BL_ONE;
        doAlpha = true;
    }
    else
    {
        diag.reportBackendErr(rootCall.m_loc, "GX backend doesn't handle '%s' root",
                              rootCall.m_call.m_name.c_str());
        return;
    }

    /* Follow Color Chain */
    const IR::Instruction& colorRoot =
    ir.m_instructions.at(rootCall.m_call.m_argInstIdxs.at(0));
    RecursiveTraceColor(ir, diag, colorRoot);

    /* Follow Alpha Chain */
    if (doAlpha)
    {
        const IR::Instruction& alphaRoot =
        ir.m_instructions.at(rootCall.m_call.m_argInstIdxs.at(1));
        RecursiveTraceAlpha(ir, diag, alphaRoot);

        /* Ensure Alpha reaches end of chain */
        if (m_alphaTraceStage >= 0)
            for (unsigned i=m_alphaTraceStage+1 ; i<m_tevCount ; ++i)
                m_tevs[i].m_alpha[3] = CA_APREV;
    }

    /* Resolve lazy color/alpha regs */
    if (m_cRegLazy)
    {
        for (int i=0 ; i<int(m_tevCount) ; ++i)
        {
            TEVStage& stage = m_tevs[i];
            if (stage.m_regOut == TEVLAZY)
            {
                int picked = pickCLazy(diag, stage.m_loc, i);
                stage.m_regOut = TevRegID(TEVREG0 + picked);
                for (int j=i+1 ; j<int(m_tevCount) ; ++j)
                {
                    TEVStage& nstage = m_tevs[j];
                    if (nstage.m_lazyCInIdx == stage.m_lazyOutIdx)
                        for (int c=0 ; c<4 ; ++c)
                            if (nstage.m_color[c] == CC_LAZY)
                                nstage.m_color[c] = TevColorArg(CC_C0 + picked * 2);
                    if (nstage.m_lazyAInIdx == stage.m_lazyOutIdx)
                        for (int c=0 ; c<4 ; ++c)
                            if (nstage.m_alpha[c] == CA_LAZY)
                                nstage.m_alpha[c] = TevAlphaArg(CA_A0 + picked);
                }
            }
        }
    }

    printf("");
}

}
}
