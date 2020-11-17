
    // Old code:
    for( auto &pathNode : paths ) {
        if( pathNode.first != destinationType )
            continue;

        // Our type directly decays into the destination type
        weight += pathNode.second.pathWeight;
        if( weight>weightLimit )
            throw ExpressionImpl::Base::ExpressionTooExpensive();

        ExpressionImpl::ExpressionMetadata metadata;
        metadata.type = destinationType;
        metadata.valueRange = pathNode.second.descriptor->calcVrp(
                srcMetadata.type.get(), destinationType.get(), srcMetadata.valueRange, true );

        return std::unique_ptr<CastChain>( new CastChain( nullptr, *pathNode.second.descriptor, metadata ) );
    }

    LookupContext::CastsList upperHook = lookupContext.allCastsTo( destinationType );
    if( !upperHook && pendingCandidates.empty() )
        return nullptr;

    /* We are looking for the shortest (least number of casts) path between sourceType and destinationType,
     * that *also* has the least weight. To that end, we are doing a breadth first search (BFS) of the graph
     * of all possible casts.
     *
     * Future enhancement:
     * Since BFS trees tend to be rather wide, we minimize the search by doing two searches, one starting with
     * sourceType, and another ending at destinationType. Any point the two trees meet is a valid sequence of
     * casts.
     *
     * We start with all casts leading to destinationType (upper tree). If not found, we search the lower tree.
     * The casts in the lower tree must be distinct, or the path would have been closed by the upper one.
     * We then continue populating the upper tree in the casts leading to the types covered by the previous step
     * and so on until we run out of new types to reach or we find a path.
     */

    paths.emplace( destinationType, Junction{ .pathWeight=weight } );

    Weight effectiveLimit = ExpressionImpl::Base::NoWeightLimit;
    for( ; upperHook ; ++upperHook ) {
        ASSERT( upperHook->sourceType != srcMetadata.type )<<
                "Single step cast should have been handled by fastpath "<<srcMetadata.type<<" to "<<destinationType;

        if( implicit && upperHook->whenPossible==LookupContext::CastDescriptor::ImplicitCastAllowed::Never )
            continue;

        Weight interimWeight( upperHook->weight );
        if( interimWeight<=weightLimit ) {
            paths.emplace( upperHook->sourceType, Junction{ .descriptor=upperHook.get(), .pathWeight=interimWeight } );
            pendingCandidates.emplace_back( upperHook->sourceType );
        } else {
            // Since the list is sorted, there will be no valid candidates past this one
            break;
        }
    }

    unsigned chainLength = 1;
    while( !pendingCandidates.empty() && validPaths.empty() ) {
        candidates = std::move(pendingCandidates);
        ASSERT( pendingCandidates.size()==0 );
        chainLength++;

        for( const auto &candidate : candidates ) {
            upperHook = lookupContext.allCastsTo( candidate );

            for( ; upperHook ; ++upperHook ) {
                if(
                        implicit &&
                        upperHook->whenPossible==LookupContext::CastDescriptor::ImplicitCastAllowed::Never )
                {
                    continue;
                }

                Weight interimWeight = Weight(upperHook->weight) + paths.at( upperHook->destType ).pathWeight;

                if( interimWeight>effectiveLimit ) {
                    // Since the list is sorted, there will be no valid candidates past this one
                    break;
                }

                if( upperHook->sourceType == srcMetadata.type ) {
                    if( interimWeight<effectiveLimit ) {
                        // validPaths has paths that are too expensive
                        validPaths.clear();
                    }

                    effectiveLimit = interimWeight;
                    validPaths.emplace_back(
                            Junction{ .descriptor = upperHook.get(), .pathWeight = interimWeight } );

                    break;
                }

                auto sourceIter = paths.find( upperHook->sourceType );
                if( sourceIter!=paths.end() ) {
                    // We've already reached this type. See if this time it's a better path
                    if( sourceIter->second.pathWeight>interimWeight ) {
                        sourceIter->second.pathWeight = interimWeight;
                        sourceIter->second.descriptor = upperHook.get();
                    }

                    continue;
                }

                paths.emplace(
                        upperHook->sourceType,
                        Junction{ .descriptor = upperHook.get(), .pathWeight=interimWeight } );
                pendingCandidates.emplace_back( upperHook->sourceType );
            }
        }
    }

    if( validPaths.size()>1 )
        throw AmbiguousCast(srcMetadata.type, destinationType, implicit, line, col);

    if( validPaths.empty() )
        return nullptr;

    weight += validPaths[0].pathWeight;
    if( weight>weightLimit )
        throw ExpressionImpl::Base::ExpressionTooExpensive();

    const LookupContext::CastDescriptor *currentCast = validPaths[0].descriptor;
    ASSERT( currentCast!=nullptr );
    ASSERT( currentCast->sourceType == srcMetadata.type );
    std::unique_ptr<CastChain> ret( new CastChain( nullptr, *currentCast ) );

    const ExpressionImpl::ExpressionMetadata *lastMetadata = &srcMetadata;
    goto skipSettingRet;
    do {
        ret = std::unique_ptr<CastChain>( new CastChain( std::move(ret), *currentCast ) );

skipSettingRet:
        ASSERT( currentCast->calcVrp != nullptr );
        ret->metadata.valueRange = currentCast->calcVrp(
                currentCast->sourceType.get(), currentCast->destType.get(),
                lastMetadata->valueRange, implicit );
        if( ! ret->metadata.valueRange )
            return nullptr;

        lastMetadata = &ret->metadata;
        currentCast = paths.at( currentCast->destType ).descriptor;
    } while( currentCast!=nullptr );

    return ret;
