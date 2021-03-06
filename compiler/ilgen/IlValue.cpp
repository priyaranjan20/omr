/*******************************************************************************
 *
 * (c) Copyright IBM Corp. 2016, 2017
 *
 *  This program and the accompanying materials are made available
 *  under the terms of the Eclipse Public License v1.0 and
 *  Apache License v2.0 which accompanies this distribution.
 *
 *      The Eclipse Public License is available at
 *      http://www.eclipse.org/legal/epl-v10.html
 *
 *      The Apache License v2.0 is available at
 *      http://www.opensource.org/licenses/apache2.0.php
 *
 * Contributors:
 *    Multiple authors (IBM Corp.) - initial implementation and documentation
 ******************************************************************************/



#include "compile/Compilation.hpp"
#include "il/Block.hpp"
#include "il/Node.hpp"
#include "il/Node_inlines.hpp"
#include "il/Symbol.hpp"
#include "il/symbol/AutomaticSymbol.hpp"
#include "il/SymbolReference.hpp"
#include "ilgen/IlValue.hpp" // must follow include for compile/Compilation.hpp for TR_Memory
#include "ilgen/MethodBuilder.hpp"


OMR::IlValue::IlValue(TR::Node *node, TR::TreeTop *treeTop, TR::Block *block, TR::MethodBuilder *methodBuilder)
   : _id(methodBuilder->getNextValueID()),
     _nodeThatComputesValue(node),
     _treeTopThatAnchorsValue(treeTop),
     _blockThatComputesValue(block),
     _methodBuilder(methodBuilder),
     _symRefThatCanBeUsedInOtherBlocks(0)
   {
   }

TR::DataType
OMR::IlValue::getDataType()
   {
   return _nodeThatComputesValue->getDataType();
   }

void
OMR::IlValue::storeToAuto()
   {
   if (_symRefThatCanBeUsedInOtherBlocks == NULL)
      {
      TR::Compilation *comp = TR::comp();

      // first use from another block, need to create symref and insert store tree where node  was computed
      TR::SymbolReference *symRef = comp->getSymRefTab()->createTemporary(_methodBuilder->methodSymbol(), _nodeThatComputesValue->getDataType());
      symRef->getSymbol()->setNotCollected();
      char *name = (char *) comp->trMemory()->allocateHeapMemory((2+10+1) * sizeof(char)); // 2 ("_T") + max 10 digits + trailing zero
      sprintf(name, "_T%u", symRef->getCPIndex());
      symRef->getSymbol()->getAutoSymbol()->setName(name);

      // create store and its treetop
      TR::Node *storeNode = TR::Node::createStore(symRef, _nodeThatComputesValue);
      TR::TreeTop *prevTreeTop = _treeTopThatAnchorsValue->getPrevTreeTop();
      TR::TreeTop *newTree = TR::TreeTop::create(comp, storeNode);
      newTree->insertNewTreeTop(prevTreeTop, _treeTopThatAnchorsValue);

      _treeTopThatAnchorsValue->unlink(true);

      _treeTopThatAnchorsValue = newTree;
      _symRefThatCanBeUsedInOtherBlocks = symRef;
      }
   }

TR::Node *
OMR::IlValue::load(TR::Block *block)
   {
   if (_blockThatComputesValue == block)
      return _nodeThatComputesValue;

   storeToAuto();
   return TR::Node::createLoad(_symRefThatCanBeUsedInOtherBlocks);
   }

void
OMR::IlValue::storeOver(TR::IlValue *value, TR::Block *block)
   {
   if (value->_blockThatComputesValue == block && _blockThatComputesValue == block)
      {
      // probably not very common scenario, but if both values are in the same block as current
      // then storeOver just needs to update node pointer
      _nodeThatComputesValue = value->_nodeThatComputesValue;
      }
   else
      {
      // more commonly, if any value is in another block or this use will be in a different block, first ensure this value is
      // stored to an auto symbol
      // NOTE this may mean nodes may be stored to more than one auto, but that's ok
      storeToAuto();

      // then make sure value has been stored to the same auto
      TR::Node *store = TR::Node::createStore(_symRefThatCanBeUsedInOtherBlocks, value->_nodeThatComputesValue);
      TR::TreeTop *tt = TR::TreeTop::create(TR::comp(), store);
      block->append(tt);

      // finally, subsequent uses of this value should load the auto, so update the node pointer
      _nodeThatComputesValue = TR::Node::createLoad(_symRefThatCanBeUsedInOtherBlocks);
      }
   }
